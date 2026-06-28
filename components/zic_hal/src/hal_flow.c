/*
 * hal_flow.c
 * Flow HAL – PCNT pulse counter for Hall-effect flow meters
 *
 * Uses the ESP-IDF v5.x pcnt_unit / pcnt_channel API.
 * Overflow handling: PCNT hardware counter is 16-bit; we accumulate
 * overflow events into a 32-bit software counter.
 */

#include "hal.h"
#include "esp_log.h"
#include "driver/pulse_cnt.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "hal_flow";

/* GPIO per channel (matches hal.h constants) */
static const int s_flow_pins[HAL_FLOW_CHANNEL_COUNT] = {
    ZIC_FLOW_PCNT_PIN_0,
    ZIC_FLOW_PCNT_PIN_1,
};

/* Per-channel state */
typedef struct
{
    pcnt_unit_handle_t   unit;
    pcnt_channel_handle_t channel;
    uint32_t             overflow_count;  /**< accumulated 16-bit overflows */
    SemaphoreHandle_t    lock;
} flow_ch_t;

static flow_ch_t s_ch[HAL_FLOW_CHANNEL_COUNT];
static bool      s_initialized = false;

/*
 * PCNT counter range must straddle zero in ESP-IDF v6.
 * We use [-1, 32767] and accumulate software overflows when reaching high limit.
 */
#define PCNT_HIGH_LIMIT  32767
#define PCNT_LOW_LIMIT   -1
#define PCNT_COUNTER_SPAN (PCNT_HIGH_LIMIT - PCNT_LOW_LIMIT + 1)

static bool IRAM_ATTR pcnt_overflow_cb(pcnt_unit_handle_t unit,
                                        const pcnt_watch_event_data_t *edata,
                                        void *user_ctx)
{
    flow_ch_t *ch = (flow_ch_t *)user_ctx;
    BaseType_t woken = pdFALSE;

    /* Take lock from ISR context (will not block) */
    if (xSemaphoreTakeFromISR(ch->lock, &woken) == pdTRUE)
    {
        ch->overflow_count += PCNT_COUNTER_SPAN;
        xSemaphoreGiveFromISR(ch->lock, &woken);
    }

    portYIELD_FROM_ISR(woken);
    return false;  /* no high-priority wakeup needed */
}

hal_result_t hal_flow_init(void)
{
    if (s_initialized)
    {
        return HAL_OK;
    }

    memset(s_ch, 0, sizeof(s_ch));

    for (int i = 0; i < HAL_FLOW_CHANNEL_COUNT; i++)
    {
        s_ch[i].lock = xSemaphoreCreateMutex();
        if (s_ch[i].lock == NULL)
        {
            ESP_LOGE(TAG, "Failed to create mutex for channel %d", i);
            return HAL_IO_ERROR;
        }

        pcnt_unit_config_t unit_cfg = {
            .high_limit = PCNT_HIGH_LIMIT,
            .low_limit  = PCNT_LOW_LIMIT,
        };
        if (pcnt_new_unit(&unit_cfg, &s_ch[i].unit) != ESP_OK)
        {
            ESP_LOGE(TAG, "pcnt_new_unit failed for channel %d", i);
            return HAL_IO_ERROR;
        }

        pcnt_chan_config_t chan_cfg = {
            .edge_gpio_num = s_flow_pins[i],
            .level_gpio_num = -1,  /* no level input */
        };
        if (pcnt_new_channel(s_ch[i].unit, &chan_cfg, &s_ch[i].channel) != ESP_OK)
        {
            ESP_LOGE(TAG, "pcnt_new_channel failed for pin %d", s_flow_pins[i]);
            return HAL_IO_ERROR;
        }

        /* Count on rising edge */
        pcnt_channel_set_edge_action(s_ch[i].channel,
                                     PCNT_CHANNEL_EDGE_ACTION_INCREASE,  /* rising */
                                     PCNT_CHANNEL_EDGE_ACTION_HOLD);      /* falling */

        /* Overflow watch-point */
        pcnt_unit_add_watch_point(s_ch[i].unit, PCNT_HIGH_LIMIT);

        pcnt_event_callbacks_t cbs = {
            .on_reach = pcnt_overflow_cb,
        };
        pcnt_unit_register_event_callbacks(s_ch[i].unit, &cbs, &s_ch[i]);

        pcnt_unit_enable(s_ch[i].unit);
        pcnt_unit_clear_count(s_ch[i].unit);

        ESP_LOGI(TAG, "Flow channel %d ready (GPIO %d)", i, s_flow_pins[i]);
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Flow HAL initialized");
    return HAL_OK;
}

hal_result_t hal_flow_start(hal_flow_channel_t channel)
{
    if (!s_initialized)
    {
        return HAL_NOT_INITIALIZED;
    }
    if (channel >= HAL_FLOW_CHANNEL_COUNT)
    {
        return HAL_INVALID_PARAMETER;
    }
    pcnt_unit_start(s_ch[channel].unit);
    return HAL_OK;
}

hal_result_t hal_flow_stop(hal_flow_channel_t channel)
{
    if (!s_initialized)
    {
        return HAL_NOT_INITIALIZED;
    }
    if (channel >= HAL_FLOW_CHANNEL_COUNT)
    {
        return HAL_INVALID_PARAMETER;
    }
    pcnt_unit_stop(s_ch[channel].unit);
    return HAL_OK;
}

hal_result_t hal_flow_read(hal_flow_channel_t channel, uint32_t *count)
{
    if (!s_initialized)
    {
        return HAL_NOT_INITIALIZED;
    }
    if (channel >= HAL_FLOW_CHANNEL_COUNT || count == NULL)
    {
        return HAL_INVALID_PARAMETER;
    }

    int hw_count = 0;
    pcnt_unit_get_count(s_ch[channel].unit, &hw_count);

    xSemaphoreTake(s_ch[channel].lock, portMAX_DELAY);
    int64_t total = (int64_t)s_ch[channel].overflow_count + (int64_t)hw_count;
    if (total < 0)
    {
        total = 0;
    }
    *count = (uint32_t)total;
    xSemaphoreGive(s_ch[channel].lock);

    return HAL_OK;
}

hal_result_t hal_flow_reset(hal_flow_channel_t channel)
{
    if (!s_initialized)
    {
        return HAL_NOT_INITIALIZED;
    }
    if (channel >= HAL_FLOW_CHANNEL_COUNT)
    {
        return HAL_INVALID_PARAMETER;
    }

    xSemaphoreTake(s_ch[channel].lock, portMAX_DELAY);
    pcnt_unit_clear_count(s_ch[channel].unit);
    s_ch[channel].overflow_count = 0;
    xSemaphoreGive(s_ch[channel].lock);

    return HAL_OK;
}

hal_result_t hal_flow_read_reset(hal_flow_channel_t channel, uint32_t *count)
{
    if (!s_initialized)
    {
        return HAL_NOT_INITIALIZED;
    }
    if (channel >= HAL_FLOW_CHANNEL_COUNT || count == NULL)
    {
        return HAL_INVALID_PARAMETER;
    }

    int hw_count = 0;
    pcnt_unit_get_count(s_ch[channel].unit, &hw_count);

    xSemaphoreTake(s_ch[channel].lock, portMAX_DELAY);
    int64_t total = (int64_t)s_ch[channel].overflow_count + (int64_t)hw_count;
    if (total < 0)
    {
        total = 0;
    }
    *count = (uint32_t)total;
    s_ch[channel].overflow_count = 0;
    pcnt_unit_clear_count(s_ch[channel].unit);
    xSemaphoreGive(s_ch[channel].lock);

    return HAL_OK;
}
