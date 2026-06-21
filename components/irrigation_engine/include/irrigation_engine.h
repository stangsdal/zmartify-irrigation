#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "zone_manager.h"
#include "zic_state_machine.h"

typedef struct {
    zic_controller_t controller;
    zone_manager_t zone_manager;
} irrigation_engine_t;

void irrigation_engine_init(irrigation_engine_t *engine);
bool irrigation_engine_start_zone(irrigation_engine_t *engine, uint8_t zone_id, uint32_t runtime_seconds);
bool irrigation_engine_stop_zone(irrigation_engine_t *engine, uint8_t zone_id);
bool irrigation_engine_stop_all(irrigation_engine_t *engine);
