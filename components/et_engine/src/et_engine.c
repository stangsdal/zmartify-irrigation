#include "et_engine.h"

#include <math.h>

void et_engine_compute(const et_input_t *input, et_output_t *output)
{
    if (input == 0 || output == 0) {
        return;
    }

    *output = (et_output_t){0};
    if (input->temperature_c < -50.0f || input->temperature_c > 60.0f ||
        input->humidity_pct < 0.0f || input->humidity_pct > 100.0f ||
        input->wind_speed_mps < 0.0f || input->solar_radiation_mj_m2 < 0.0f ||
        input->elevation_m < -500.0f || input->elevation_m > 5000.0f) {
        return;
    }

    float temperature_term = input->temperature_c + 237.3f;
    float saturation_vapor_pressure =
        0.6108f * expf((17.27f * input->temperature_c) / temperature_term);
    float actual_vapor_pressure = saturation_vapor_pressure * input->humidity_pct / 100.0f;
    float vapor_pressure_slope =
        4098.0f * saturation_vapor_pressure / (temperature_term * temperature_term);
    float pressure_ratio = (293.0f - 0.0065f * input->elevation_m) / 293.0f;
    float atmospheric_pressure = 101.3f * powf(pressure_ratio, 5.26f);
    float psychrometric_constant = 0.000665f * atmospheric_pressure;
    float net_radiation = input->solar_radiation_mj_m2 * 0.77f;

    float radiation_term = 0.408f * vapor_pressure_slope * net_radiation;
    float aerodynamic_term = psychrometric_constant *
        (900.0f / (input->temperature_c + 273.0f)) * input->wind_speed_mps *
        (saturation_vapor_pressure - actual_vapor_pressure);
    float denominator = vapor_pressure_slope + psychrometric_constant *
        (1.0f + 0.34f * input->wind_speed_mps);
    float daily_et = denominator > 0.0f ? (radiation_term + aerodynamic_term) / denominator : 0.0f;
    output->daily_et_mm = daily_et > 0.0f ? daily_et : 0.0f;
    output->weekly_et_mm = output->daily_et_mm * 7.0f;
}

uint32_t et_engine_adjust_runtime_seconds(uint32_t base_runtime_seconds,
                                          float et_factor,
                                          float crop_factor,
                                          float seasonal_factor)
{
    float adjusted = (float)base_runtime_seconds * et_factor * crop_factor * seasonal_factor;
    if (adjusted < 0.0f) {
        return 0;
    }

    return (uint32_t)adjusted;
}
