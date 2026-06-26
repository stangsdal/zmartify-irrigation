/*
 * pressure_manager.c
 * Pressure Manager v5.0 – ADS1115 supervision
 *
 * Conversion: voltage (mV) → bar
 *   pressure_bar = (voltage_mv - offset_mv) / mv_per_bar
 *
 * Sensor: 0.5 V = 0 bar, 4.5 V = 10 bar → 400 mV/bar
 */

#include "pressure_manager.h"
#include "hal.h"
#include "config_manager.h"
#include "event_bus.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "pres_mgr";

#define SAMPLE_PERIOD_MS      250u
#define AVG_WINDOW            5u
#define COLLAPSE_DELTA_BAR    1.5f  /**< Drop of this much in one second = collapse */

static pressure_reading_t  s_reading;
static float               s_avg_buf[AVG_WINDOW];
static uint8_t             s_avg_idx    = 0;
static float               s_prev_bar   = -1.0f;
static SemaphoreHandle_t   s_lock       = NULL;
static bool                s_initialized = false;

static float moving_avg(float v)
{
    s_avg_buf[s_avg_idx % AVG_WINDOW] = v;
    s_avg_idx++;
    float sum = 0.0f;
    uint8_t n = (s_avg_idx < AVG_WINDOW) ? s_avg_idx : AVG_WINDOW;
    for (int i = 0; i < n; i++) sum += s_avg_buf[i];
    return sum / n;
}

static float voltage_to_bar(int32_t mv)
{
    config_hydraulic_t hyd;
    float offset_mv   = 500.0f;
    float mv_per_bar  = 400.0f;

    if (config_get_hydraulics(&hyd) == CFG_OK)
    {
        offset_mv  = hyd.pressure_offset_mv;
        mv_per_bar = hyd.pressure_mv_per_bar;
    }

    if (mv_per_bar <= 0.0f) return 0.0f;
    float bar = ((float)mv - offset_mv) / mv_per_bar;
    return (bar < 0.0f) ? 0.0f : bar;
}

static void pressure_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "Pressure Manager task running");

    uint8_t sample_accumulator = 0;

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_MS));

        int32_t mv = 0;
        hal_result_t hr = hal_pressure_read_voltage(HAL_ADC_CHANNEL_0,
                                                    HAL_ADC_PGA_4V096, &mv);

        pressure_state_t state = PRESSURE_STATE_OK;
        float bar = 0.0f;

        if (hr != HAL_OK)
        {
            state = PRESSURE_STATE_FAULT;
        }
        else
        {
            bar = voltage_to_bar(mv);
            float avg = moving_avg(bar);

            config_alarms_t alm;
            config_get_alarms(&alm);
            float min_bar = (float)alm.pressure_low_mbar  / 1000.0f;
            float max_bar = (float)alm.pressure_high_mbar / 1000.0f;

            if (avg < min_bar && min_bar > 0.0f)
            {
                state = PRESSURE_STATE_LOW;
            }
            else if (avg > max_bar && max_bar > 0.0f)
            {
                state = PRESSURE_STATE_HIGH;
            }
            else if (s_prev_bar >= 0.0f && (s_prev_bar - avg) > COLLAPSE_DELTA_BAR)
            {
                state = PRESSURE_STATE_COLLAPSE;
                ESP_LOGW(TAG, "Pressure collapse: %.2f → %.2f bar", s_prev_bar, avg);
            }

            xSemaphoreTake(s_lock, portMAX_DELAY);
            s_reading.pressure_bar = bar;
            s_reading.avg_bar      = avg;
            s_reading.raw_mv       = mv;
            s_reading.state        = state;
            s_reading.timestamp_ms = (uint32_t)(esp_timer_get_time() / 1000);
            xSemaphoreGive(s_lock);

            s_prev_bar = avg;
        }

        /* Publish every 4th sample (~1 Hz) to avoid flooding event bus */
        if (++sample_accumulator >= 4)
        {
            sample_accumulator = 0;
            uint8_t payload[4];
            uint32_t bar_x1000 = (uint32_t)(s_reading.avg_bar * 1000.0f);
            payload[0] = (uint8_t)(bar_x1000 >> 24);
            payload[1] = (uint8_t)(bar_x1000 >> 16);
            payload[2] = (uint8_t)(bar_x1000 >>  8);
            payload[3] = (uint8_t)(bar_x1000);

            event_id_e ev = (state == PRESSURE_STATE_OK)
                          ? EVENT_PRESSURE_UPDATED
                          : EVENT_PRESSURE_OUT_OF_BOUNDS;
            event_bus_publish(ev, 0,
                              (state == PRESSURE_STATE_OK)
                                ? EVENT_PRIORITY_NORMAL
                                : EVENT_PRIORITY_CRITICAL,
                              0, payload, sizeof(payload));

            if (state != PRESSURE_STATE_OK)
            {
                ESP_LOGW(TAG, "Pressure fault state=%d bar=%.2f", (int)state,
                         s_reading.avg_bar);
            }
        }
    }
}

bool pressure_manager_init(void)
{
    if (s_initialized) return true;

    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return false;

    memset(&s_reading, 0, sizeof(s_reading));
    memset(s_avg_buf, 0, sizeof(s_avg_buf));

    if (xTaskCreate(pressure_task, "pres_mgr", 3072, NULL, 9, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "Task create failed");
        return false;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Pressure Manager initialized");
    return true;
}

bool pressure_manager_get_reading(pressure_reading_t *out)
{
    if (!s_initialized || !out) return false;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    *out = s_reading;
    xSemaphoreGive(s_lock);
    return true;
}

float pressure_manager_get_bar(void)
{
    return s_reading.avg_bar;
}

pressure_state_t pressure_manager_get_state(void)
{
    return s_reading.state;
}
