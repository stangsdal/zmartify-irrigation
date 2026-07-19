#include "valve_diagnostics.h"

#include <stddef.h>

static bool open_response_present(const valve_diagnostics_t *diagnostics,
                                  const valve_diag_observation_t *observation)
{
    return observation->flow_valid &&
        observation->flow_lpm_x100 >= diagnostics->config.minimum_flow_lpm_x100;
}

bool valve_diagnostics_init(valve_diagnostics_t *diagnostics,
                            const valve_diag_config_t *config)
{
    if (diagnostics == NULL || config == NULL ||
        config->open_response_timeout_ms == 0u ||
        config->close_response_timeout_ms == 0u ||
        (config->minimum_flow_lpm_x100 == 0u &&
         config->minimum_pressure_mbar == 0u)) {
        return false;
    }

    diagnostics->config = *config;
    diagnostics->status = VALVE_DIAG_IDLE;
    diagnostics->state_since_ms = 0u;
    diagnostics->active_zone_id = 0u;
    diagnostics->commanded_open = false;
    return true;
}

void valve_diagnostics_command_open(valve_diagnostics_t *diagnostics,
                                    uint8_t zone_id,
                                    uint64_t now_ms)
{
    if (diagnostics == NULL || zone_id == 0u) {
        return;
    }
    diagnostics->status = VALVE_DIAG_WAITING_OPEN_RESPONSE;
    diagnostics->state_since_ms = now_ms;
    diagnostics->active_zone_id = zone_id;
    diagnostics->commanded_open = true;
}

void valve_diagnostics_command_close(valve_diagnostics_t *diagnostics,
                                     uint64_t now_ms)
{
    if (diagnostics == NULL) {
        return;
    }
    diagnostics->status = VALVE_DIAG_WAITING_CLOSE_RESPONSE;
    diagnostics->state_since_ms = now_ms;
    diagnostics->commanded_open = false;
}

valve_diag_status_t valve_diagnostics_evaluate(
    valve_diagnostics_t *diagnostics,
    const valve_diag_observation_t *observation,
    uint64_t now_ms)
{
    if (diagnostics == NULL || observation == NULL) {
        return VALVE_DIAG_SENSOR_UNAVAILABLE;
    }

    bool open_available = observation->flow_valid;
    bool open_response = open_response_present(diagnostics, observation);
    bool close_available = observation->flow_valid;
    bool flow_present = close_available &&
        observation->flow_lpm_x100 >= diagnostics->config.minimum_flow_lpm_x100;
    uint64_t elapsed_ms = now_ms - diagnostics->state_since_ms;

    if (diagnostics->status == VALVE_DIAG_WAITING_OPEN_RESPONSE) {
        if (open_response) {
            diagnostics->status = VALVE_DIAG_OPEN_CONFIRMED;
        } else if (elapsed_ms >= diagnostics->config.open_response_timeout_ms) {
            if (!open_available) {
                diagnostics->status = VALVE_DIAG_SENSOR_UNAVAILABLE;
            } else if (observation->pressure_valid &&
                       observation->pressure_mbar >= diagnostics->config.minimum_pressure_mbar) {
                diagnostics->status = VALVE_DIAG_LIKELY_STUCK_CLOSED;
            } else {
                diagnostics->status = VALVE_DIAG_NO_RESPONSE;
            }
        }
    } else if (diagnostics->status == VALVE_DIAG_WAITING_CLOSE_RESPONSE) {
        if (close_available && !flow_present) {
            diagnostics->status = VALVE_DIAG_IDLE;
            diagnostics->active_zone_id = 0u;
        } else if (elapsed_ms >= diagnostics->config.close_response_timeout_ms) {
            diagnostics->status = close_available
                ? VALVE_DIAG_LIKELY_STUCK_OPEN
                : VALVE_DIAG_SENSOR_UNAVAILABLE;
        }
    } else if (diagnostics->status == VALVE_DIAG_IDLE && flow_present) {
        diagnostics->status = VALVE_DIAG_WAITING_CLOSE_RESPONSE;
        diagnostics->state_since_ms = now_ms;
    } else if (diagnostics->status == VALVE_DIAG_SENSOR_UNAVAILABLE &&
               (diagnostics->commanded_open ? open_available : close_available)) {
        if (diagnostics->commanded_open) {
            if (open_response) {
                diagnostics->status = VALVE_DIAG_OPEN_CONFIRMED;
            } else if (observation->pressure_valid &&
                       observation->pressure_mbar >= diagnostics->config.minimum_pressure_mbar) {
                diagnostics->status = VALVE_DIAG_LIKELY_STUCK_CLOSED;
            } else {
                diagnostics->status = VALVE_DIAG_NO_RESPONSE;
            }
        } else if (flow_present) {
            diagnostics->status = VALVE_DIAG_LIKELY_STUCK_OPEN;
        } else {
            diagnostics->status = VALVE_DIAG_IDLE;
            diagnostics->active_zone_id = 0u;
        }
    }

    return diagnostics->status;
}

bool valve_diagnostics_requires_shutdown(valve_diag_status_t status)
{
    return status == VALVE_DIAG_NO_RESPONSE ||
        status == VALVE_DIAG_LIKELY_STUCK_CLOSED ||
        status == VALVE_DIAG_LIKELY_STUCK_OPEN;
}
