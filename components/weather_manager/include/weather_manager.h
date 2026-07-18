#pragma once

#include <stdbool.h>

typedef struct {
    float rain_mm_last_24h;
    float rain_probability_pct;
    float humidity_pct;
    float uv_index;
    float temperature_c;
    float wind_speed_mps;
} weather_snapshot_t;

typedef struct {
    bool skip_watering;
    bool reduce_watering;
    bool increase_watering;
    bool suspend_watering;
    bool block_watering;
} weather_decision_t;

void weather_manager_evaluate(const weather_snapshot_t *snapshot, weather_decision_t *decision);
