#pragma once

#include <stdint.h>

typedef struct {
    float temperature_c;
    float humidity_pct;
    float wind_speed_mps;
    float solar_radiation_mj_m2;
} et_input_t;

typedef struct {
    float daily_et_mm;
    float weekly_et_mm;
} et_output_t;

void et_engine_compute(const et_input_t *input, et_output_t *output);
uint32_t et_engine_adjust_runtime_seconds(uint32_t base_runtime_seconds,
                                          float et_factor,
                                          float crop_factor,
                                          float seasonal_factor);
