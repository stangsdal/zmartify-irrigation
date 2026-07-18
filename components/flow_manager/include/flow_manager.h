#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "alarm_manager.h"

typedef struct {
    uint32_t baseline_lpm_x100;
    uint32_t current_lpm_x100;
} flow_manager_t;

void flow_manager_init(flow_manager_t *manager);
void flow_manager_set_baseline(flow_manager_t *manager, uint32_t baseline_lpm_x100);
void flow_manager_update(flow_manager_t *manager,
                         uint32_t current_lpm_x100,
                         alarm_manager_t *alarm_manager);
