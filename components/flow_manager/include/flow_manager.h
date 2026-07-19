#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "alarm_manager.h"

typedef enum {
    FLOW_SOURCE_DIRECT_FALLBACK = 0,
    FLOW_SOURCE_WATERSENSOR,
} flow_source_t;

typedef struct {
    flow_source_t source;
    uint32_t flow_lpm_x100;
    uint32_t measurement_age_ms;
    uint64_t volume_total_ml;
    uint64_t pulse_count_total;
    bool valid;
} flow_measurement_t;

typedef struct {
    uint32_t baseline_lpm_x100;
    uint32_t current_lpm_x100;
    flow_source_t source;
    uint32_t measurement_age_ms;
    uint64_t volume_total_ml;
    uint64_t pulse_count_total;
    bool measurement_valid;
    uint64_t low_flow_since_ms;
    uint64_t high_flow_since_ms;
    uint64_t unexpected_flow_since_ms;
    uint8_t warning_deviation_pct;
    uint8_t critical_deviation_pct;
} flow_manager_t;

typedef struct {
    uint32_t low_flow_lpm_x100;
    uint32_t high_flow_lpm_x100;
    uint32_t no_flow_timeout_ms;
    uint32_t high_flow_duration_ms;
    uint32_t active_max_age_ms;
    uint32_t idle_max_age_ms;
} flow_supervision_config_t;

void flow_manager_init(flow_manager_t *manager);
void flow_manager_set_baseline(flow_manager_t *manager, uint32_t baseline_lpm_x100);
void flow_manager_set_deviation_limits(flow_manager_t *manager,
                                       uint8_t warning_deviation_pct,
                                       uint8_t critical_deviation_pct);
void flow_manager_update(flow_manager_t *manager,
                         uint32_t current_lpm_x100,
                         alarm_manager_t *alarm_manager);
bool flow_manager_update_measurement(flow_manager_t *manager,
                                     const flow_measurement_t *measurement,
                                     alarm_manager_t *alarm_manager);
bool flow_manager_supervise(flow_manager_t *manager,
                            const flow_measurement_t *measurement,
                            const flow_supervision_config_t *config,
                            bool irrigation_active,
                            uint64_t now_ms,
                            alarm_manager_t *alarm_manager);
