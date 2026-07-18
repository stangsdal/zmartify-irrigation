#pragma once

#include <stdint.h>

#include "alarm_manager.h"

typedef struct {
    uint32_t min_pressure_mbar;
    uint32_t max_pressure_mbar;
    uint32_t current_pressure_mbar;
    uint64_t low_pressure_since_ms;
    uint64_t high_pressure_since_ms;
    bool measurement_valid;
} pressure_manager_t;

typedef struct {
    uint32_t low_pressure_mbar;
    uint32_t high_pressure_mbar;
    uint32_t critical_duration_ms;
} pressure_supervision_config_t;

void pressure_manager_init(pressure_manager_t *manager,
                           uint32_t min_pressure_mbar,
                           uint32_t max_pressure_mbar);
void pressure_manager_update(pressure_manager_t *manager,
                             uint32_t current_pressure_mbar,
                             alarm_manager_t *alarm_manager);
void pressure_manager_clear_alarms(alarm_manager_t *alarm_manager);
bool pressure_manager_supervise(pressure_manager_t *manager,
                                const pressure_supervision_config_t *config,
                                bool measurement_valid,
                                uint32_t current_pressure_mbar,
                                bool irrigation_active,
                                uint64_t now_ms,
                                alarm_manager_t *alarm_manager);
