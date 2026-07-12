#include "et_engine.h"

void et_engine_compute(const et_input_t *input, et_output_t *output)
{
    if (input == 0 || output == 0) {
        return;
    }

    float et_proxy = (0.0023f * (input->temperature_c + 17.8f) * (input->solar_radiation_mj_m2 + 1.0f));
    output->daily_et_mm = et_proxy;
    output->weekly_et_mm = et_proxy * 7.0f;
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
