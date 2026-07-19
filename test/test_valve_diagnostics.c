#include <assert.h>
#include <stdio.h>

#include "valve_diagnostics.h"

static valve_diagnostics_t initialized_diagnostics(void)
{
    valve_diagnostics_t diagnostics;
    valve_diag_config_t config = {
        .open_response_timeout_ms = 3000,
        .close_response_timeout_ms = 2000,
        .minimum_flow_lpm_x100 = 200,
        .minimum_pressure_mbar = 500,
    };
    assert(valve_diagnostics_init(&diagnostics, &config));
    return diagnostics;
}

static void test_open_response_and_likely_stuck_closed(void)
{
    valve_diagnostics_t diagnostics = initialized_diagnostics();
    valve_diag_observation_t no_response = {
        .flow_valid = true,
        .flow_lpm_x100 = 0,
        .pressure_valid = true,
        .pressure_mbar = 700,
    };

    valve_diagnostics_command_open(&diagnostics, 3, 1000);
    assert(valve_diagnostics_evaluate(&diagnostics, &no_response, 3999) ==
           VALVE_DIAG_WAITING_OPEN_RESPONSE);
    assert(valve_diagnostics_evaluate(&diagnostics, &no_response, 4000) ==
           VALVE_DIAG_LIKELY_STUCK_CLOSED);

    diagnostics = initialized_diagnostics();
    valve_diagnostics_command_open(&diagnostics, 3, 1000);
        no_response.flow_lpm_x100 = 500;
    assert(valve_diagnostics_evaluate(&diagnostics, &no_response, 1500) ==
           VALVE_DIAG_OPEN_CONFIRMED);

        diagnostics = initialized_diagnostics();
        valve_diagnostics_command_open(&diagnostics, 3, 1000);
            no_response.flow_lpm_x100 = 0;
            no_response.pressure_mbar = 100;
            assert(valve_diagnostics_evaluate(&diagnostics, &no_response, 4000) ==
                VALVE_DIAG_NO_RESPONSE);
            assert(valve_diagnostics_requires_shutdown(VALVE_DIAG_NO_RESPONSE));
            assert(valve_diagnostics_requires_shutdown(VALVE_DIAG_LIKELY_STUCK_CLOSED));
}

static void test_unavailable_is_not_a_valve_fault(void)
{
    valve_diagnostics_t diagnostics = initialized_diagnostics();
    valve_diag_observation_t unavailable = {0};

    valve_diagnostics_command_open(&diagnostics, 2, 1000);
    assert(valve_diagnostics_evaluate(&diagnostics, &unavailable, 4000) ==
           VALVE_DIAG_SENSOR_UNAVAILABLE);
    assert(!valve_diagnostics_requires_shutdown(VALVE_DIAG_SENSOR_UNAVAILABLE));

    valve_diag_observation_t recovered = {
        .flow_valid = true,
        .flow_lpm_x100 = 500,
    };
    assert(valve_diagnostics_evaluate(&diagnostics, &recovered, 4500) ==
           VALVE_DIAG_OPEN_CONFIRMED);
}

static void test_close_response_and_likely_stuck_open(void)
{
    valve_diagnostics_t diagnostics = initialized_diagnostics();
    valve_diag_observation_t flowing = {
        .flow_valid = true,
        .flow_lpm_x100 = 500,
    };
    valve_diag_observation_t stopped = {
        .flow_valid = true,
        .flow_lpm_x100 = 0,
    };

    valve_diagnostics_command_open(&diagnostics, 4, 1000);
    assert(valve_diagnostics_evaluate(&diagnostics, &flowing, 1100) ==
           VALVE_DIAG_OPEN_CONFIRMED);
    valve_diagnostics_command_close(&diagnostics, 5000);
    assert(valve_diagnostics_evaluate(&diagnostics, &flowing, 6999) ==
           VALVE_DIAG_WAITING_CLOSE_RESPONSE);
    assert(valve_diagnostics_evaluate(&diagnostics, &flowing, 7000) ==
           VALVE_DIAG_LIKELY_STUCK_OPEN);
    assert(valve_diagnostics_requires_shutdown(VALVE_DIAG_LIKELY_STUCK_OPEN));

    diagnostics = initialized_diagnostics();
    valve_diagnostics_command_close(&diagnostics, 5000);
    assert(valve_diagnostics_evaluate(&diagnostics, &stopped, 5100) == VALVE_DIAG_IDLE);
}

static void test_unexpected_idle_flow(void)
{
    valve_diagnostics_t diagnostics = initialized_diagnostics();
    valve_diag_observation_t flowing = {
        .flow_valid = true,
        .flow_lpm_x100 = 500,
    };

    assert(valve_diagnostics_evaluate(&diagnostics, &flowing, 1000) ==
           VALVE_DIAG_WAITING_CLOSE_RESPONSE);
    assert(valve_diagnostics_evaluate(&diagnostics, &flowing, 3000) ==
           VALVE_DIAG_LIKELY_STUCK_OPEN);
}

int main(void)
{
    test_open_response_and_likely_stuck_closed();
    test_unavailable_is_not_a_valve_fault();
    test_close_response_and_likely_stuck_open();
    test_unexpected_idle_flow();
    puts("valve diagnostics tests passed");
    return 0;
}
