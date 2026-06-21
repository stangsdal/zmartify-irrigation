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

int main(void)
{
    test_flow_deviation_thresholds();
    test_pressure_thresholds();
    return 0;
}
