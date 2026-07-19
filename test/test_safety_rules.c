#include <assert.h>

#include "alarm_manager.h"
#include "flow_manager.h"
#include "pressure_manager.h"

static void test_flow_deviation_thresholds(void)
{
    alarm_manager_t alarms;
    flow_manager_t flow;

    alarm_manager_init(&alarms);
    flow_manager_init(&flow);
    flow_manager_set_baseline(&flow, 1000);
    flow_manager_set_deviation_limits(&flow, 15, 30);

    flow_manager_update(&flow, 1160, &alarms);
    assert(alarm_manager_is_active(&alarms, ZIC_ALARM_HIGH_FLOW));

    flow_manager_update(&flow, 1350, &alarms);
    assert(alarm_manager_is_active(&alarms, ZIC_ALARM_LEAK_DETECTED));
}

static void test_pressure_thresholds(void)
{
    alarm_manager_t alarms;
    pressure_manager_t pressure;

    alarm_manager_init(&alarms);
    pressure_manager_init(&pressure, 2000, 7000);

    pressure_manager_update(&pressure, 1500, &alarms);
    assert(alarm_manager_is_active(&alarms, ZIC_ALARM_LOW_PRESSURE));

    pressure_manager_update(&pressure, 7200, &alarms);
    assert(alarm_manager_is_active(&alarms, ZIC_ALARM_HIGH_PRESSURE));
}

static void test_pressure_escalation(void)
{
    alarm_manager_t alarms;
    pressure_manager_t pressure;
    const pressure_supervision_config_t config = {
        .low_pressure_mbar = 2000,
        .high_pressure_mbar = 7000,
        .critical_duration_ms = 5000,
    };

    alarm_manager_init(&alarms);
    pressure_manager_init(&pressure, 2000, 7000);
    assert(pressure_manager_supervise(&pressure, &config, true, 1500, true, 1000, &alarms));
    assert(alarm_manager_is_active(&alarms, ZIC_ALARM_LOW_PRESSURE));
    assert(!alarm_manager_is_active(&alarms, ZIC_ALARM_PRESSURE_COLLAPSE));
    assert(pressure_manager_supervise(&pressure, &config, true, 1500, true, 5999, &alarms));
    assert(!alarm_manager_is_active(&alarms, ZIC_ALARM_PRESSURE_COLLAPSE));
    assert(pressure_manager_supervise(&pressure, &config, true, 1500, true, 6000, &alarms));
    assert(alarm_manager_is_active(&alarms, ZIC_ALARM_PRESSURE_COLLAPSE));
    assert(alarm_manager_has_severity(&alarms, ZIC_ALARM_CRITICAL));

    assert(pressure_manager_supervise(&pressure, &config, true, 3500, true, 6100, &alarms));
    assert(!alarm_manager_is_active(&alarms, ZIC_ALARM_LOW_PRESSURE));
    assert(!alarm_manager_is_active(&alarms, ZIC_ALARM_PRESSURE_COLLAPSE));
    assert(!pressure_manager_supervise(&pressure, &config, false, 0, true, 6200, &alarms));
    assert(!pressure.measurement_valid);
}

static void test_context_aware_flow_supervision(void)
{
    alarm_manager_t alarms;
    flow_manager_t flow;
    const flow_supervision_config_t config = {
        .low_flow_lpm_x100 = 200,
        .high_flow_lpm_x100 = 20000,
        .no_flow_timeout_ms = 30000,
        .high_flow_duration_ms = 10000,
        .active_max_age_ms = 1500,
        .idle_max_age_ms = 5000,
    };
    flow_measurement_t measurement = {
        .source = FLOW_SOURCE_WATERSENSOR,
        .valid = true,
    };

    alarm_manager_init(&alarms);
    flow_manager_init(&flow);

    assert(flow_manager_supervise(&flow, &measurement, &config, false, 1000, &alarms));
    assert(!alarm_manager_is_active(&alarms, ZIC_ALARM_LEAK_DETECTED));

    assert(flow_manager_supervise(&flow, &measurement, &config, true, 2000, &alarms));
    assert(flow_manager_supervise(&flow, &measurement, &config, true, 31999, &alarms));
    assert(!alarm_manager_is_active(&alarms, ZIC_ALARM_LOW_FLOW));
    assert(flow_manager_supervise(&flow, &measurement, &config, true, 32000, &alarms));
    assert(alarm_manager_is_active(&alarms, ZIC_ALARM_LOW_FLOW));

    measurement.flow_lpm_x100 = 300;
    assert(flow_manager_supervise(&flow, &measurement, &config, true, 32100, &alarms));
    assert(!alarm_manager_is_active(&alarms, ZIC_ALARM_LOW_FLOW));

    measurement.measurement_age_ms = 1501;
    measurement.flow_lpm_x100 = 0;
    assert(!flow_manager_supervise(&flow, &measurement, &config, true, 33000, &alarms));
    assert(!flow.measurement_valid);
    assert(!alarm_manager_is_active(&alarms, ZIC_ALARM_LOW_FLOW));

    measurement.measurement_age_ms = 0;
    measurement.flow_lpm_x100 = 21000;
    assert(flow_manager_supervise(&flow, &measurement, &config, true, 34000, &alarms));
    assert(flow_manager_supervise(&flow, &measurement, &config, true, 43999, &alarms));
    assert(!alarm_manager_is_active(&alarms, ZIC_ALARM_HIGH_FLOW));
    assert(flow_manager_supervise(&flow, &measurement, &config, true, 44000, &alarms));
    assert(alarm_manager_is_active(&alarms, ZIC_ALARM_HIGH_FLOW));
    assert(alarm_manager_has_severity(&alarms, ZIC_ALARM_CRITICAL));

    measurement.flow_lpm_x100 = 300;
    assert(flow_manager_supervise(&flow, &measurement, &config, true, 44100, &alarms));
    assert(!alarm_manager_is_active(&alarms, ZIC_ALARM_HIGH_FLOW));

    assert(flow_manager_supervise(&flow, &measurement, &config, false, 40000, &alarms));
    assert(flow_manager_supervise(&flow, &measurement, &config, false, 49999, &alarms));
    assert(!alarm_manager_is_active(&alarms, ZIC_ALARM_LEAK_DETECTED));
    assert(flow_manager_supervise(&flow, &measurement, &config, false, 50000, &alarms));
    assert(alarm_manager_is_active(&alarms, ZIC_ALARM_LEAK_DETECTED));
}

int main(void)
{
    test_flow_deviation_thresholds();
    test_pressure_thresholds();
    test_pressure_escalation();
    test_context_aware_flow_supervision();
    return 0;
}
