/*
 * weather_manager.c
 * Weather Manager v5.0
 */

#include "weather_manager.h"
#include "hal.h"
#include "config_manager.h"
#include "event_bus.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "weather_mgr";

#define NVS_KEY_RAIN_DELAY_END  "rain_delay_end"
#define DEFAULT_SKIP_RAIN_MM         5.0f
#define DEFAULT_SKIP_RAIN_PROB_PCT   70.0f
#define DEFAULT_HIGH_WIND_MPS        10.0f
#define DEFAULT_FREEZE_TEMP_C        2.0f
#define DEFAULT_REDUCE_HUMID_PCT     85.0f
#define DEFAULT_INCREASE_UV          7.0f
#define DEFAULT_INCREASE_TEMP_C      32.0f
#define DEFAULT_REDUCE_FACTOR        0.7f
#define DEFAULT_INCREASE_FACTOR      1.3f

static weather_snapshot_t s_snapshot;
static SemaphoreHandle_t  s_lock              = NULL;
static uint32_t           s_rain_delay_end_epoch = 0;
static bool               s_initialized       = false;

static uint32_t current_epoch(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000000ULL);
}

void weather_set_rain_delay(uint16_t hours)
{
    if (hours == 0)
    {
        s_rain_delay_end_epoch = 0;
        hal_storage_write_u32(NVS_KEY_RAIN_DELAY_END, 0);
        ESP_LOGI(TAG, "Rain delay cleared");
        event_bus_publish(EVENT_WEATHER_UPDATED, 0, EVENT_PRIORITY_NORMAL, 0, NULL, 0);
    }
    else
    {
        s_rain_delay_end_epoch = current_epoch() + (uint32_t)hours * 3600u;
        hal_storage_write_u32(NVS_KEY_RAIN_DELAY_END, s_rain_delay_end_epoch);
        ESP_LOGI(TAG, "Rain delay: %u hours", hours);
        event_bus_publish(EVENT_RAIN_DETECTED, 0, EVENT_PRIORITY_HIGH, 0, NULL, 0);
    }
}

bool weather_rain_delay_active(void)
{
    if (s_rain_delay_end_epoch == 0) return false;
    if (current_epoch() >= s_rain_delay_end_epoch)
    {
        s_rain_delay_end_epoch = 0;
        hal_storage_write_u32(NVS_KEY_RAIN_DELAY_END, 0);
        return false;
    }
    return true;
}

uint32_t weather_rain_delay_remaining_s(void)
{
    if (!weather_rain_delay_active()) return 0;
    uint32_t now = current_epoch();
    return (s_rain_delay_end_epoch > now) ? s_rain_delay_end_epoch - now : 0;
}

void weather_get_decision(weather_decision_t *out)
{
    if (!out) return;
    memset(out, 0, sizeof(*out));
    out->adjustment_factor = 1.0f;

    xSemaphoreTake(s_lock, portMAX_DELAY);
    weather_snapshot_t snap = s_snapshot;
    xSemaphoreGive(s_lock);

    if (!snap.data_valid) return;

    if (snap.temperature_c <= DEFAULT_FREEZE_TEMP_C && snap.temperature_c > -99.0f)
    {
        out->block_watering = true;
        out->adjustment_factor = 0.0f;
        return;
    }
    if (snap.wind_speed_mps >= DEFAULT_HIGH_WIND_MPS)
    {
        out->suspend_watering = true;
        out->adjustment_factor = 0.0f;
        return;
    }
    if (snap.rain_mm_last_24h >= DEFAULT_SKIP_RAIN_MM ||
        snap.rain_probability_pct >= DEFAULT_SKIP_RAIN_PROB_PCT ||
        snap.forecast_rain_mm >= DEFAULT_SKIP_RAIN_MM)
    {
        out->skip_watering = true;
        out->adjustment_factor = 0.0f;
        return;
    }
    if (snap.humidity_pct >= DEFAULT_REDUCE_HUMID_PCT)
    {
        out->reduce_watering = true;
        out->adjustment_factor = DEFAULT_REDUCE_FACTOR;
        return;
    }
    if (snap.temperature_c >= DEFAULT_INCREASE_TEMP_C || snap.uv_index >= DEFAULT_INCREASE_UV)
    {
        out->increase_watering = true;
        out->adjustment_factor = DEFAULT_INCREASE_FACTOR;
    }
}

bool weather_irrigation_allowed(void)
{
    if (weather_rain_delay_active()) return false;
    weather_decision_t d;
    weather_get_decision(&d);
    return !d.block_watering && !d.suspend_watering && !d.skip_watering;
}

void weather_update(const weather_snapshot_t *snapshot)
{
    if (!snapshot) return;

    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_snapshot = *snapshot;
    if (s_snapshot.updated_epoch == 0) s_snapshot.updated_epoch = current_epoch();
    s_snapshot.data_valid = true;
    xSemaphoreGive(s_lock);

    ESP_LOGI(TAG, "Weather: T=%.1fC H=%.0f%% W=%.1fm/s Rain=%.1fmm",
             snapshot->temperature_c, snapshot->humidity_pct,
             snapshot->wind_speed_mps, snapshot->rain_mm_last_24h);

    if (snapshot->rain_mm_last_24h >= DEFAULT_SKIP_RAIN_MM && !weather_rain_delay_active())
        weather_set_rain_delay(24);

    event_bus_publish(EVENT_WEATHER_UPDATED, 0, EVENT_PRIORITY_NORMAL, 0, NULL, 0);
}

void weather_get_snapshot(weather_snapshot_t *out)
{
    if (!out) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    *out = s_snapshot;
    xSemaphoreGive(s_lock);
}

bool weather_manager_init(void)
{
    if (s_initialized) return true;
    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return false;

    memset(&s_snapshot, 0, sizeof(s_snapshot));
    s_snapshot.temperature_c = -99.0f;
    s_snapshot.data_valid    = false;

    hal_storage_read_u32(NVS_KEY_RAIN_DELAY_END, &s_rain_delay_end_epoch, 0);

    s_initialized = true;
    ESP_LOGI(TAG, "Weather Manager initialized");
    return true;
}
