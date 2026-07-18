#include "weather_manager.h"

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
