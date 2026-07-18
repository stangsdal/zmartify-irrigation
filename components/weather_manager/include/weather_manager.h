#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#define WEATHER_MAX_CACHE_AGE_SECONDS (24u * 60u * 60u)

typedef struct {
    uint32_t timestamp;
    float rain_mm_last_24h;
    float rain_probability_pct;
    float humidity_pct;
    float uv_index;
    float temperature_c;
    float wind_speed_mps;
    float solar_radiation_mj_m2;
    bool valid;
} weather_snapshot_t;

typedef struct {
    bool skip_watering;
    bool reduce_watering;
    bool increase_watering;
    bool suspend_watering;
    bool block_watering;
} weather_decision_t;

typedef bool (*weather_load_fn)(void *context, void *data, size_t *length);
typedef bool (*weather_save_fn)(void *context, const void *data, size_t length);

typedef struct {
    weather_snapshot_t snapshot;
    uint8_t consecutive_failures;
    bool has_snapshot;
    weather_load_fn load;
    weather_save_fn save;
    void *persistence_context;
} weather_manager_t;

typedef struct {
    uint32_t runtime_seconds;
    uint16_t adjustment_percent;
    float effective_rain_mm;
    bool skip_watering;
    bool weather_applied;
} weather_adjustment_t;

void weather_manager_init(weather_manager_t *manager);
void weather_manager_set_persistence(weather_manager_t *manager,
                                     weather_load_fn load,
                                     weather_save_fn save,
                                     void *context);
bool weather_manager_restore(weather_manager_t *manager);
bool weather_manager_update(weather_manager_t *manager, const weather_snapshot_t *snapshot);
void weather_manager_record_failure(weather_manager_t *manager);
bool weather_manager_get_snapshot(const weather_manager_t *manager,
                                  uint32_t now,
                                  weather_snapshot_t *out);
void weather_manager_evaluate(const weather_snapshot_t *snapshot, weather_decision_t *decision);
bool weather_manager_adjust_runtime(const weather_snapshot_t *snapshot,
                                    uint32_t now,
                                    uint32_t base_runtime_seconds,
                                    float reference_et_mm,
                                    uint8_t crop_coefficient_x100,
                                    uint16_t seasonal_percent,
                                    weather_adjustment_t *adjustment);
