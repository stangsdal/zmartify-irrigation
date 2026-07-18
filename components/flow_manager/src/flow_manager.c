#include "flow_manager.h"

static uint32_t abs_diff_u32(uint32_t a, uint32_t b)
{
    return (a > b) ? (a - b) : (b - a);
}

void flow_manager_init(flow_manager_t *manager)
{
    if (manager == 0) {
        return;
    }

    manager->baseline_lpm_x100 = 0;
    manager->current_lpm_x100 = 0;
}

void flow_manager_set_baseline(flow_manager_t *manager, uint32_t baseline_lpm_x100)
{
    if (manager == 0) {
        return;
    }

    manager->baseline_lpm_x100 = baseline_lpm_x100;
}

void flow_manager_update(flow_manager_t *manager,
                         uint32_t current_lpm_x100,
                         alarm_manager_t *alarm_manager)
{
    if (manager == 0 || alarm_manager == 0) {
        return;
    }

    manager->current_lpm_x100 = current_lpm_x100;

    if (manager->baseline_lpm_x100 == 0) {
        return;
    }

    uint32_t deviation = abs_diff_u32(manager->current_lpm_x100, manager->baseline_lpm_x100);
    uint32_t deviation_pct_x100 = (deviation * 10000U) / manager->baseline_lpm_x100;

    if (deviation_pct_x100 >= 3000U) {
        alarm_manager_raise(alarm_manager, ZIC_ALARM_LEAK_DETECTED, ZIC_ALARM_CRITICAL);
        return;
    }

    if (deviation_pct_x100 >= 1500U) {
        alarm_manager_raise(alarm_manager, ZIC_ALARM_HIGH_FLOW, ZIC_ALARM_WARNING);
        return;
    }

    alarm_manager_clear(alarm_manager, ZIC_ALARM_HIGH_FLOW);
    alarm_manager_clear(alarm_manager, ZIC_ALARM_LEAK_DETECTED);
}
