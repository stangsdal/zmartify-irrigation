#include "diagnostics_policy.h"

#include <stddef.h>

#define DIAG_HEAP_CRITICAL_PCT 90u
#define DIAG_STACK_CRITICAL_BYTES 512u

static diag_status_t worst(diag_status_t left, diag_status_t right)
{
    return left > right ? left : right;
}

diag_policy_result_t diagnostics_policy_evaluate(const diag_policy_input_t *input)
{
    diag_policy_result_t result = {
        .overall = DIAG_STATUS_CRITICAL,
        .hydraulics = DIAG_STATUS_UNAVAILABLE,
        .communications = DIAG_STATUS_UNAVAILABLE,
        .storage = DIAG_STATUS_UNAVAILABLE,
        .runtime = DIAG_STATUS_CRITICAL,
        .ota_acceptable = false,
    };
    if (input == NULL) {
        return result;
    }

    if (input->flow_sensor_available && input->pressure_sensor_available) {
        result.hydraulics = DIAG_STATUS_HEALTHY;
    } else if (input->flow_sensor_available || input->pressure_sensor_available) {
        result.hydraulics = DIAG_STATUS_DEGRADED;
    }

    if (input->mqtt_connected && input->time_synchronized) {
        result.communications = DIAG_STATUS_HEALTHY;
    } else if (input->mqtt_connected || input->time_synchronized) {
        result.communications = DIAG_STATUS_DEGRADED;
    }

    if (input->storage_ready) {
        result.storage = input->storage_last_write_ok
            ? DIAG_STATUS_HEALTHY : DIAG_STATUS_CRITICAL;
    }

    bool stack_critical = input->control_stack_free_bytes < DIAG_STACK_CRITICAL_BYTES ||
        input->telemetry_stack_free_bytes < DIAG_STACK_CRITICAL_BYTES ||
        (input->watersensor_stack_free_bytes != 0u &&
         input->watersensor_stack_free_bytes < DIAG_STACK_CRITICAL_BYTES);
    if (!input->core_ready || input->heap_utilisation_pct >= DIAG_HEAP_CRITICAL_PCT ||
        stack_critical || input->critical_alarms > 0u) {
        result.runtime = DIAG_STATUS_CRITICAL;
    } else if (input->event_drops > 0u) {
        result.runtime = DIAG_STATUS_DEGRADED;
    } else {
        result.runtime = DIAG_STATUS_HEALTHY;
    }

    result.overall = worst(result.runtime, result.storage);
    result.overall = worst(result.overall, result.hydraulics);
    result.overall = worst(result.overall, result.communications);
    result.ota_acceptable = result.runtime != DIAG_STATUS_CRITICAL &&
        result.storage == DIAG_STATUS_HEALTHY;
    return result;
}

const char *diagnostics_status_name(diag_status_t status)
{
    switch (status) {
    case DIAG_STATUS_HEALTHY:
        return "healthy";
    case DIAG_STATUS_DEGRADED:
        return "degraded";
    case DIAG_STATUS_UNAVAILABLE:
        return "unavailable";
    case DIAG_STATUS_CRITICAL:
    default:
        return "critical";
    }
}
