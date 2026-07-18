#include "weather_manager.h"

#include <string.h>

#define WEATHER_CACHE_MAGIC 0x5A574541u
#define WEATHER_CACHE_VERSION 1u

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t reserved;
    uint32_t crc32;
    weather_snapshot_t snapshot;
} weather_cache_t;

static uint32_t crc32_compute(const void *data, size_t length)
{
    const uint8_t *bytes = (const uint8_t *)data;
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t index = 0; index < length; ++index) {
        crc ^= bytes[index];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

static float clamp_float(float value, float minimum, float maximum)
{
    if (value < minimum) {
        return minimum;
    }
    return value > maximum ? maximum : value;
}

void weather_manager_init(weather_manager_t *manager)
{
    if (manager == 0) {
        return;
    }
    *manager = (weather_manager_t){0};
}

void weather_manager_set_persistence(weather_manager_t *manager,
                                     weather_load_fn load,
                                     weather_save_fn save,
                                     void *context)
{
    if (manager == 0) {
        return;
    }
    manager->load = load;
    manager->save = save;
    manager->persistence_context = context;
}

bool weather_manager_restore(weather_manager_t *manager)
{
    if (manager == 0 || manager->load == 0) {
        return false;
    }
    weather_cache_t cache = {0};
    size_t length = sizeof(cache);
    if (!manager->load(manager->persistence_context, &cache, &length) ||
        length != sizeof(cache) || cache.magic != WEATHER_CACHE_MAGIC ||
        cache.version != WEATHER_CACHE_VERSION) {
        return false;
    }
    uint32_t stored_crc = cache.crc32;
    cache.crc32 = 0;
    if (crc32_compute(&cache, sizeof(cache)) != stored_crc) {
        return false;
    }
    weather_save_fn save = manager->save;
    manager->save = 0;
    bool restored = weather_manager_update(manager, &cache.snapshot);
    manager->save = save;
    if (!restored) {
        return false;
    }
    return true;
}

bool weather_manager_update(weather_manager_t *manager, const weather_snapshot_t *snapshot)
{
    if (manager == 0 || snapshot == 0 || !snapshot->valid || snapshot->timestamp == 0 ||
        snapshot->humidity_pct < 0.0f || snapshot->humidity_pct > 100.0f ||
        snapshot->rain_probability_pct < 0.0f || snapshot->rain_probability_pct > 100.0f ||
        snapshot->rain_mm_last_24h < 0.0f || snapshot->wind_speed_mps < 0.0f ||
        snapshot->solar_radiation_mj_m2 < 0.0f) {
        return false;
    }
    manager->snapshot = *snapshot;
    manager->consecutive_failures = 0;
    manager->has_snapshot = true;
    if (manager->save != 0) {
        weather_cache_t cache = {
            .magic = WEATHER_CACHE_MAGIC,
            .version = WEATHER_CACHE_VERSION,
            .snapshot = *snapshot,
        };
        cache.crc32 = 0;
        cache.crc32 = crc32_compute(&cache, sizeof(cache));
        (void)manager->save(manager->persistence_context, &cache, sizeof(cache));
    }
    return true;
}

void weather_manager_record_failure(weather_manager_t *manager)
{
    if (manager != 0 && manager->consecutive_failures < UINT8_MAX) {
        ++manager->consecutive_failures;
    }
}

bool weather_manager_get_snapshot(const weather_manager_t *manager,
                                  uint32_t now,
                                  weather_snapshot_t *out)
{
    if (manager == 0 || out == 0 || !manager->has_snapshot || now < manager->snapshot.timestamp ||
        now - manager->snapshot.timestamp > WEATHER_MAX_CACHE_AGE_SECONDS) {
        return false;
    }
    *out = manager->snapshot;
    return true;
}

void weather_manager_evaluate(const weather_snapshot_t *snapshot, weather_decision_t *decision)
{
    if (snapshot == 0 || decision == 0) {
        return;
    }

    decision->skip_watering = snapshot->rain_mm_last_24h >= 5.0f || snapshot->rain_probability_pct >= 70.0f;
    decision->reduce_watering = snapshot->humidity_pct >= 85.0f;
    decision->increase_watering = snapshot->uv_index >= 7.0f || snapshot->temperature_c >= 32.0f;
    decision->suspend_watering = snapshot->wind_speed_mps >= 10.0f;
    decision->block_watering = snapshot->temperature_c <= 0.0f;
}

bool weather_manager_adjust_runtime(const weather_snapshot_t *snapshot,
                                    uint32_t now,
                                    uint32_t base_runtime_seconds,
                                    float reference_et_mm,
                                    uint8_t crop_coefficient_x100,
                                    uint16_t seasonal_percent,
                                    weather_adjustment_t *adjustment)
{
    if (adjustment == 0) {
        return false;
    }
    *adjustment = (weather_adjustment_t){
        .runtime_seconds = (uint32_t)((float)base_runtime_seconds *
                                      clamp_float((float)seasonal_percent / 100.0f,
                                                  0.2f, 2.0f) + 0.5f),
        .adjustment_percent = (uint16_t)(clamp_float((float)seasonal_percent / 100.0f,
                                                     0.2f, 2.0f) * 100.0f + 0.5f),
    };
    if (snapshot == 0 || !snapshot->valid || snapshot->timestamp == 0 ||
        now < snapshot->timestamp || now - snapshot->timestamp > WEATHER_MAX_CACHE_AGE_SECONDS ||
        reference_et_mm <= 0.0f) {
        return true;
    }

    weather_decision_t decision = {0};
    weather_manager_evaluate(snapshot, &decision);
    adjustment->weather_applied = true;
    adjustment->effective_rain_mm = snapshot->rain_mm_last_24h * 0.8f;
    float crop_et_mm = reference_et_mm * ((float)crop_coefficient_x100 / 100.0f);
    float required_mm = crop_et_mm - adjustment->effective_rain_mm;
    if (decision.skip_watering || decision.suspend_watering || decision.block_watering ||
        required_mm <= 0.0f) {
        adjustment->runtime_seconds = 0;
        adjustment->adjustment_percent = 0;
        adjustment->skip_watering = true;
        return true;
    }

    float factor = (required_mm / reference_et_mm) * ((float)seasonal_percent / 100.0f);
    factor = clamp_float(factor, 0.2f, 2.0f);
    adjustment->runtime_seconds = (uint32_t)((float)base_runtime_seconds * factor + 0.5f);
    adjustment->adjustment_percent = (uint16_t)(factor * 100.0f + 0.5f);
    return true;
}
