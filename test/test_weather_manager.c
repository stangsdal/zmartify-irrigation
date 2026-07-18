#include <assert.h>
#include <string.h>

#include "weather_manager.h"

static uint8_t persisted[256];
static size_t persisted_length;

static bool load_cache(void *context, void *data, size_t *length)
{
    (void)context;
    if (persisted_length == 0 || *length < persisted_length) {
        return false;
    }
    memcpy(data, persisted, persisted_length);
    *length = persisted_length;
    return true;
}

static bool save_cache(void *context, const void *data, size_t length)
{
    (void)context;
    assert(length <= sizeof(persisted));
    memcpy(persisted, data, length);
    persisted_length = length;
    return true;
}

static weather_snapshot_t valid_snapshot(void)
{
    return (weather_snapshot_t){
        .timestamp = 1000,
        .rain_probability_pct = 20.0f,
        .humidity_pct = 55.0f,
        .temperature_c = 25.0f,
        .wind_speed_mps = 3.0f,
        .solar_radiation_mj_m2 = 18.0f,
        .valid = true,
    };
}

static void test_cache_and_failure_fallback(void)
{
    weather_manager_t manager;
    weather_manager_init(&manager);
    persisted_length = 0;
    weather_manager_set_persistence(&manager, load_cache, save_cache, 0);
    weather_snapshot_t snapshot = valid_snapshot();
    assert(weather_manager_update(&manager, &snapshot));
    weather_manager_record_failure(&manager);
    weather_manager_record_failure(&manager);
    weather_manager_record_failure(&manager);

    weather_snapshot_t cached;
    assert(weather_manager_get_snapshot(&manager, 1000 + WEATHER_MAX_CACHE_AGE_SECONDS, &cached));
    assert(!weather_manager_get_snapshot(&manager,
                                         1001 + WEATHER_MAX_CACHE_AGE_SECONDS, &cached));

    weather_manager_t restored;
    weather_manager_init(&restored);
    weather_manager_set_persistence(&restored, load_cache, save_cache, 0);
    assert(weather_manager_restore(&restored));
    assert(weather_manager_get_snapshot(&restored, 1000, &cached));

    persisted[persisted_length - 1] ^= 1u;
    assert(!weather_manager_restore(&restored));
}

static void test_runtime_adjustment_and_rain_deduction(void)
{
    weather_snapshot_t snapshot = valid_snapshot();
    weather_adjustment_t adjustment;

    assert(weather_manager_adjust_runtime(&snapshot, 1000, 600, 5.0f, 80, 100,
                                          &adjustment));
    assert(adjustment.weather_applied);
    assert(adjustment.adjustment_percent == 80);
    assert(adjustment.runtime_seconds == 480);

    snapshot.rain_mm_last_24h = 5.0f;
    assert(weather_manager_adjust_runtime(&snapshot, 1000, 600, 5.0f, 80, 100,
                                          &adjustment));
    assert(adjustment.skip_watering);
    assert(adjustment.runtime_seconds == 0);
}

static void test_bounds_and_stale_fallback(void)
{
    weather_snapshot_t snapshot = valid_snapshot();
    weather_adjustment_t adjustment;

    assert(weather_manager_adjust_runtime(&snapshot, 1000, 600, 5.0f, 20, 20,
                                          &adjustment));
    assert(adjustment.adjustment_percent == 20);
    assert(adjustment.runtime_seconds == 120);

    assert(weather_manager_adjust_runtime(&snapshot, 1000, 600, 5.0f, 200, 200,
                                          &adjustment));
    assert(adjustment.adjustment_percent == 200);
    assert(adjustment.runtime_seconds == 1200);

    assert(weather_manager_adjust_runtime(&snapshot,
                                          1001 + WEATHER_MAX_CACHE_AGE_SECONDS,
                                          600, 5.0f, 80, 100, &adjustment));
    assert(!adjustment.weather_applied);
    assert(adjustment.adjustment_percent == 100);
    assert(adjustment.runtime_seconds == 600);

    assert(weather_manager_adjust_runtime(&snapshot,
                                          1001 + WEATHER_MAX_CACHE_AGE_SECONDS,
                                          600, 5.0f, 80, 50, &adjustment));
    assert(adjustment.adjustment_percent == 50);
    assert(adjustment.runtime_seconds == 300);
}

int main(void)
{
    test_cache_and_failure_fallback();
    test_runtime_adjustment_and_rain_deduction();
    test_bounds_and_stale_fallback();
    return 0;
}