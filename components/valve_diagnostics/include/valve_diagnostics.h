#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    VALVE_DIAG_IDLE = 0,
    VALVE_DIAG_WAITING_OPEN_RESPONSE,
    VALVE_DIAG_OPEN_CONFIRMED,
    VALVE_DIAG_WAITING_CLOSE_RESPONSE,
    VALVE_DIAG_SENSOR_UNAVAILABLE,
    VALVE_DIAG_NO_RESPONSE,
    VALVE_DIAG_LIKELY_STUCK_CLOSED,
    VALVE_DIAG_LIKELY_STUCK_OPEN,
} valve_diag_status_t;

typedef struct {
    uint32_t open_response_timeout_ms;
    uint32_t close_response_timeout_ms;
    uint32_t minimum_flow_lpm_x100;
    uint32_t minimum_pressure_mbar;
} valve_diag_config_t;

typedef struct {
    bool flow_valid;
    uint32_t flow_lpm_x100;
    bool pressure_valid;
    uint32_t pressure_mbar;
} valve_diag_observation_t;

typedef struct {
    valve_diag_config_t config;
    valve_diag_status_t status;
    uint64_t state_since_ms;
    uint8_t active_zone_id;
    bool commanded_open;
} valve_diagnostics_t;

bool valve_diagnostics_init(valve_diagnostics_t *diagnostics,
                            const valve_diag_config_t *config);
void valve_diagnostics_command_open(valve_diagnostics_t *diagnostics,
                                    uint8_t zone_id,
                                    uint64_t now_ms);
void valve_diagnostics_command_close(valve_diagnostics_t *diagnostics,
                                     uint64_t now_ms);
valve_diag_status_t valve_diagnostics_evaluate(
    valve_diagnostics_t *diagnostics,
    const valve_diag_observation_t *observation,
    uint64_t now_ms);
bool valve_diagnostics_requires_shutdown(valve_diag_status_t status);
