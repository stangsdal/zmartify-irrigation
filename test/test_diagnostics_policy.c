#include <assert.h>
#include <string.h>

#include "diagnostics_policy.h"

static diag_policy_input_t healthy_input(void)
{
    return (diag_policy_input_t) {
        .core_ready = true,
        .storage_ready = true,
        .storage_last_write_ok = true,
        .flow_sensor_available = true,
        .pressure_sensor_available = true,
        .mqtt_connected = true,
        .time_synchronized = true,
        .control_stack_free_bytes = 4096,
        .telemetry_stack_free_bytes = 2048,
        .watersensor_stack_free_bytes = 2048,
    };
}

static void test_health_classes(void)
{
    diag_policy_input_t input = healthy_input();
    diag_policy_result_t result = diagnostics_policy_evaluate(&input);
    assert(result.overall == DIAG_STATUS_HEALTHY);
    assert(result.ota_acceptable);

    input.mqtt_connected = false;
    result = diagnostics_policy_evaluate(&input);
    assert(result.communications == DIAG_STATUS_DEGRADED);
    assert(result.overall == DIAG_STATUS_DEGRADED);
    assert(result.ota_acceptable);

    input.time_synchronized = false;
    result = diagnostics_policy_evaluate(&input);
    assert(result.communications == DIAG_STATUS_UNAVAILABLE);
    assert(result.overall == DIAG_STATUS_UNAVAILABLE);
    assert(result.ota_acceptable);

    input = healthy_input();
    input.flow_sensor_available = false;
    input.pressure_sensor_available = false;
    result = diagnostics_policy_evaluate(&input);
    assert(result.hydraulics == DIAG_STATUS_UNAVAILABLE);
    assert(result.ota_acceptable);
}

static void test_critical_release_gates(void)
{
    diag_policy_input_t input = healthy_input();
    input.critical_alarms = 1;
    diag_policy_result_t result = diagnostics_policy_evaluate(&input);
    assert(result.overall == DIAG_STATUS_CRITICAL);
    assert(!result.ota_acceptable);

    input = healthy_input();
    input.storage_last_write_ok = false;
    result = diagnostics_policy_evaluate(&input);
    assert(result.storage == DIAG_STATUS_CRITICAL);
    assert(!result.ota_acceptable);

    input = healthy_input();
    input.control_stack_free_bytes = 511;
    result = diagnostics_policy_evaluate(&input);
    assert(result.runtime == DIAG_STATUS_CRITICAL);
    assert(!result.ota_acceptable);

    input = healthy_input();
    input.event_drops = 1;
    result = diagnostics_policy_evaluate(&input);
    assert(result.runtime == DIAG_STATUS_DEGRADED);
    assert(result.ota_acceptable);
}

static void test_status_names_and_null_input(void)
{
    assert(strcmp(diagnostics_status_name(DIAG_STATUS_HEALTHY), "healthy") == 0);
    assert(strcmp(diagnostics_status_name(DIAG_STATUS_DEGRADED), "degraded") == 0);
    assert(strcmp(diagnostics_status_name(DIAG_STATUS_UNAVAILABLE), "unavailable") == 0);
    assert(strcmp(diagnostics_status_name(DIAG_STATUS_CRITICAL), "critical") == 0);
    assert(diagnostics_policy_evaluate(NULL).overall == DIAG_STATUS_CRITICAL);
}

int main(void)
{
    test_health_classes();
    test_critical_release_gates();
    test_status_names_and_null_input();
    return 0;
}
