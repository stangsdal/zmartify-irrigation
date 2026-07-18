#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "zone_manager.h"
#include "zic_state_machine.h"

typedef enum {
    IRRIGATION_PHASE_IDLE = 0,
    IRRIGATION_PHASE_MASTER_OPEN_DELAY,
    IRRIGATION_PHASE_RUNNING,
    IRRIGATION_PHASE_MASTER_CLOSE_DELAY,
    IRRIGATION_PHASE_FAULT,
} irrigation_phase_t;

typedef struct {
    zic_controller_t controller;
    zone_manager_t zone_manager;
    irrigation_phase_t phase;
    uint64_t deadline_ms;
    uint32_t requested_runtime_seconds;
    uint8_t active_zone_id;
    uint8_t active_relay_index;
} irrigation_engine_t;

void irrigation_engine_init(irrigation_engine_t *engine);
bool irrigation_engine_start_zone(irrigation_engine_t *engine,
                                  uint8_t zone_id,
                                  uint8_t relay_index,
                                  uint32_t runtime_seconds,
                                  uint64_t now_ms);
bool irrigation_engine_stop_zone(irrigation_engine_t *engine, uint8_t zone_id);
bool irrigation_engine_stop_all(irrigation_engine_t *engine);
bool irrigation_engine_tick(irrigation_engine_t *engine, uint64_t now_ms);
bool irrigation_engine_is_idle(const irrigation_engine_t *engine);
uint32_t irrigation_engine_remaining_seconds(const irrigation_engine_t *engine, uint64_t now_ms);
