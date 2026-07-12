#pragma once

#include <stdint.h>

#include "alarm_manager.h"

typedef struct {
    uint32_t min_pressure_mbar;
    uint32_t max_pressure_mbar;
    uint32_t current_pressure_mbar;
} pressure_manager_t;

void pressure_manager_init(pressure_manager_t *manager,
                           uint32_t min_pressure_mbar,
                           uint32_t max_pressure_mbar);
void pressure_manager_update(pressure_manager_t *manager,
                             uint32_t current_pressure_mbar,
                             alarm_manager_t *alarm_manager);
