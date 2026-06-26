/*
 * flow_manager.c
 * Flow Manager v5.0 – DN50 Hall-effect flow meter supervision
 *
 * Sampling: 1 second (hal_flow_read_reset gives pulses in interval)
 * L/min = pulses_per_second * 60 / pulses_per_litre
 * Moving average: 5-sample FIFO
 */

#include "flow_manager.h"
#include "hal.h"
#include "config_manager.h"
#include "event_bus.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "flow_mgr";

/* ─── Constants ──────────────────────────────────────────────────────── */

#define SAMPLE_PERIOD_MS      1000u   /**< 1-second sample interval */
#define AVG_WINDOW            5u      /**< Moving-average window size */
#define NO_FLOW_TIMEOUT_S     15u     /**< Seconds before no-flow alarm */
#define IDLE_FLOW_THRESHOLD_LPM  0.5f /**< L/min considered "unexpected flow" */
#define DEFAULT_PULSES_PER_LITRE 450.0f

/* ─── State ───────────────────────────────────────────────────────────── */

static flow_reading_t  s_reading;
static float           s_avg_buf[AVG_WINDOW];
static uint8_t         s_avg_idx    = 0;
static uint8_t         s_active_zone = 0;        /* 0 = system idle */
static uint32_t        s_no_flow_counter = 0;    /* seconds with no flow while active */
static SemaphoreHandle_t s_lock    = NULL;
static bool            s_initialized = false;

/* ─── Helpers ─────────────────────────────────────────────────────────── */

static float moving_average_push(float new_val)
{
    s_avg_buf[s_avg_idx % AVG_WINDOW] = new_val;
    s_avg_idx++;
    float sum = 0.0f;
    uint8_t count = (s_avg_idx < AVG_WINDOW) ? s_avg_idx : AVG_WINDOW;
    for (int i = 0; i < count; i++)
    {
        sum += s_avg_buf[i];
    }
    return sum / count;
}

static float get_pulses_per_litre(void)
{
    config_hydraulic_t hyd;
    if (config_get_hydraulics(&hyd) == CFG_OK && hyd.flow_pulses_per_litre > 0.0f)
    {
        return hyd.flow_pulses_per_litre;
    }
    return DEFAULT_PULSES_PER_LITRE;
}

/* ─── Sampling task ───────────────────────────────────────────────────── */

static void flow_task(void *arg)
{
    (void)arg;
    ESP_LOGI(TAG, "Flow Manager task running");

    /* Start PCNT counting */
    hal_flow_start(HAL_FLOW_CHANNEL_0);

    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(SAMPLE_PERIOD_MS));

        /* Read and reset counter for this interval (1 second = pulses/s) */
        uint32_t pulses = 0;
        hal_flow_read_reset(HAL_FLOW_CHANNEL_0, &pulses);

        float ppl = get_pulses_per_litre();
        float litres_this_second = (ppl > 0.0f) ? (float)pulses / ppl : 0.0f;
        float rate_lpm = litres_this_second * 60.0f;
        float avg_lpm  = moving_average_push(rate_lpm);

        /* Get alarm thresholds from config */
        config_alarms_t alm;
        config_get_alarms(&alm);
        float high_lpm = (float)alm.flow_high_lpm_x10 / 10.0f;
        float low_lpm  = (float)alm.flow_low_lpm_x10  / 10.0f;

        flow_state_t state = FLOW_STATE_OK;

        if (s_active_zone == 0)
        {
            /* Idle supervision: detect unexpected flow */
            if (avg_lpm > IDLE_FLOW_THRESHOLD_LPM)
            {
                state = FLOW_STATE_UNEXPECTED;
                ESP_LOGW(TAG, "Unexpected flow while idle: %.1f L/min", avg_lpm);
            }
            s_no_flow_counter = 0;
        }
        else
        {
            /* Active zone supervision */
            if (avg_lpm > high_lpm && high_lpm > 0.0f)
            {
                state = FLOW_STATE_HIGH;
                ESP_LOGW(TAG, "Zone %u high flow: %.1f L/min (max %.1f)",
                         s_active_zone, avg_lpm, high_lpm);
            }
            else if (avg_lpm < low_lpm && low_lpm > 0.0f)
            {
                s_no_flow_counter++;
                if (s_no_flow_counter >= alm.no_flow_timeout_s)
                {
                    state = FLOW_STATE_NO_FLOW;
                    ESP_LOGW(TAG, "Zone %u no/low flow for %lu s",
                             s_active_zone, (unsigned long)s_no_flow_counter);
                    s_no_flow_counter = 0;
                }
            }
            else
            {
                s_no_flow_counter = 0;
            }
        }

        /* Update snapshot under lock */
        xSemaphoreTake(s_lock, portMAX_DELAY);
        s_reading.rate_lpm          = rate_lpm;
        s_reading.avg_lpm           = avg_lpm;
        s_reading.pulse_count      += pulses;
        s_reading.state             = state;
        s_reading.total_litres_x10 += (uint32_t)(litres_this_second * 10.0f);
        s_reading.timestamp_ms      = (uint32_t)(esp_timer_get_time() / 1000);
        xSemaphoreGive(s_lock);

        /* Publish update event */
        uint8_t payload[4];
        uint32_t lpm_x100 = (uint32_t)(avg_lpm * 100.0f);
        payload[0] = (uint8_t)(lpm_x100 >> 24);
        payload[1] = (uint8_t)(lpm_x100 >> 16);
        payload[2] = (uint8_t)(lpm_x100 >>  8);
        payload[3] = (uint8_t)(lpm_x100);

        event_id_e ev = (state == FLOW_STATE_OK) ? EVENT_FLOW_UPDATED
                                                  : EVENT_FLOW_ANOMALY;
        event_bus_publish(ev, 0, (state == FLOW_STATE_OK)
                          ? EVENT_PRIORITY_NORMAL : EVENT_PRIORITY_CRITICAL,
                          0, payload, sizeof(payload));
    }
}

/* ─── Public API ──────────────────────────────────────────────────────── */

bool flow_manager_init(void)
{
    if (s_initialized)
    {
        return true;
    }

    s_lock = xSemaphoreCreateMutex();
    if (!s_lock)
    {
        ESP_LOGE(TAG, "Failed to create lock");
        return false;
    }

    memset(&s_reading, 0, sizeof(s_reading));
    memset(s_avg_buf, 0, sizeof(s_avg_buf));

    if (xTaskCreate(flow_task, "flow_mgr", 3072, NULL, 9, NULL) != pdPASS)
    {
        ESP_LOGE(TAG, "Task create failed");
        return false;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Flow Manager initialized");
    return true;
}

void flow_manager_set_active_zone(uint8_t zone_id)
{
    s_active_zone     = zone_id;
    s_no_flow_counter = 0;
    ESP_LOGI(TAG, "Active zone: %u", zone_id);
}

bool flow_manager_get_reading(flow_reading_t *out)
{
    if (!s_initialized || !out)
    {
        return false;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    *out = s_reading;
    xSemaphoreGive(s_lock);
    return true;
}

float flow_manager_get_rate_lpm(void)
{
    return s_reading.avg_lpm;
}

flow_state_t flow_manager_get_state(void)
{
    return s_reading.state;
}

void flow_manager_reset_lifetime(void)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_reading.total_litres_x10 = 0;
    s_reading.pulse_count      = 0;
    xSemaphoreGive(s_lock);
}
