#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    DIAG_STATUS_HEALTHY = 0,
    DIAG_STATUS_DEGRADED,
    DIAG_STATUS_UNAVAILABLE,
    DIAG_STATUS_CRITICAL,
} diag_status_t;

typedef struct {
    bool core_ready;
    bool storage_ready;
    bool storage_last_write_ok;
    bool flow_sensor_available;
    bool pressure_sensor_available;
    bool mqtt_connected;
    bool time_synchronized;
    uint8_t critical_alarms;
    uint8_t heap_utilisation_pct;
    uint32_t event_drops;
    uint32_t control_stack_free_bytes;
    uint32_t telemetry_stack_free_bytes;
    uint32_t watersensor_stack_free_bytes;
} diag_policy_input_t;

typedef struct {
    diag_status_t overall;
    diag_status_t hydraulics;
    diag_status_t communications;
    diag_status_t storage;
    diag_status_t runtime;
    bool ota_acceptable;
} diag_policy_result_t;

diag_policy_result_t diagnostics_policy_evaluate(const diag_policy_input_t *input);
const char *diagnostics_status_name(diag_status_t status);
