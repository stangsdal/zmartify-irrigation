#include <assert.h>
#include <math.h>

#include "et_engine.h"

static void test_fao56_daily_reference(void)
{
    et_input_t input = {
        .temperature_c = 25.0f,
        .humidity_pct = 50.0f,
        .wind_speed_mps = 2.0f,
        .solar_radiation_mj_m2 = 20.0f,
        .elevation_m = 0.0f,
    };
    et_output_t output;
    et_engine_compute(&input, &output);
    assert(fabsf(output.daily_et_mm - 6.06f) < 0.05f);
    assert(fabsf(output.weekly_et_mm - output.daily_et_mm * 7.0f) < 0.01f);
}

static void test_humidity_and_wind_affect_et(void)
{
    et_input_t input = {
        .temperature_c = 25.0f,
        .humidity_pct = 80.0f,
        .wind_speed_mps = 1.0f,
        .solar_radiation_mj_m2 = 20.0f,
    };
    et_output_t humid;
    et_engine_compute(&input, &humid);

    input.humidity_pct = 30.0f;
    input.wind_speed_mps = 4.0f;
    et_output_t dry_and_windy;
    et_engine_compute(&input, &dry_and_windy);
    assert(dry_and_windy.daily_et_mm > humid.daily_et_mm);
}

static void test_invalid_input_returns_zero(void)
{
    et_input_t input = {
        .temperature_c = 25.0f,
        .humidity_pct = 101.0f,
        .wind_speed_mps = 2.0f,
        .solar_radiation_mj_m2 = 20.0f,
    };
    et_output_t output = {.daily_et_mm = 42.0f, .weekly_et_mm = 42.0f};
    et_engine_compute(&input, &output);
    assert(output.daily_et_mm == 0.0f);
    assert(output.weekly_et_mm == 0.0f);
}

int main(void)
{
    test_fao56_daily_reference();
    test_humidity_and_wind_affect_et();
    test_invalid_input_returns_zero();
    return 0;
}