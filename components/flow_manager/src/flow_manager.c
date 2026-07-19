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
    manager->source = FLOW_SOURCE_DIRECT_FALLBACK;
    manager->measurement_age_ms = 0;
    manager->volume_total_ml = 0;
    manager->pulse_count_total = 0;
    manager->measurement_valid = false;
    manager->low_flow_since_ms = 0;
    manager->high_flow_since_ms = 0;
    manager->unexpected_flow_since_ms = 0;
    manager->warning_deviation_pct = 0;
    manager->critical_deviation_pct = 0;
}

void flow_manager_set_deviation_limits(flow_manager_t *manager,
                                       uint8_t warning_deviation_pct,
                                       uint8_t critical_deviation_pct)
{
    if (manager == 0 || warning_deviation_pct == 0 ||
        warning_deviation_pct >= critical_deviation_pct) {
        return;
    }
    manager->warning_deviation_pct = warning_deviation_pct;
    manager->critical_deviation_pct = critical_deviation_pct;
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
    flow_measurement_t measurement = {
        .source = FLOW_SOURCE_DIRECT_FALLBACK,
        .flow_lpm_x100 = current_lpm_x100,
        .valid = true,
    };
    (void)flow_manager_update_measurement(manager, &measurement, alarm_manager);
}

bool flow_manager_update_measurement(flow_manager_t *manager,
                                     const flow_measurement_t *measurement,
                                     alarm_manager_t *alarm_manager)
{
    if (manager == 0 || alarm_manager == 0) {
        return false;
    }
    if (measurement == 0 || !measurement->valid) {
        manager->measurement_valid = false;
        return false;
    }

    manager->current_lpm_x100 = measurement->flow_lpm_x100;
    manager->source = measurement->source;
    manager->measurement_age_ms = measurement->measurement_age_ms;
    manager->volume_total_ml = measurement->volume_total_ml;
    manager->pulse_count_total = measurement->pulse_count_total;
    manager->measurement_valid = true;

    if (manager->baseline_lpm_x100 == 0 || manager->warning_deviation_pct == 0 ||
        manager->critical_deviation_pct == 0) {
        return true;
    }

    uint32_t deviation = abs_diff_u32(manager->current_lpm_x100, manager->baseline_lpm_x100);
    uint32_t deviation_pct_x100 = (deviation * 10000U) / manager->baseline_lpm_x100;

    if (deviation_pct_x100 >= (uint32_t)manager->critical_deviation_pct * 100u) {
        alarm_manager_raise(alarm_manager, ZIC_ALARM_LEAK_DETECTED, ZIC_ALARM_CRITICAL);
        return true;
    }

    if (deviation_pct_x100 >= (uint32_t)manager->warning_deviation_pct * 100u) {
        alarm_manager_raise(alarm_manager, ZIC_ALARM_HIGH_FLOW, ZIC_ALARM_WARNING);
        return true;
    }

    alarm_manager_clear(alarm_manager, ZIC_ALARM_HIGH_FLOW);
    alarm_manager_clear(alarm_manager, ZIC_ALARM_LEAK_DETECTED);
    return true;
}

static bool duration_elapsed(uint64_t since_ms, uint64_t now_ms, uint32_t duration_ms)
{
    return since_ms != 0 && now_ms - since_ms >= duration_ms;
}

bool flow_manager_supervise(flow_manager_t *manager,
                            const flow_measurement_t *measurement,
                            const flow_supervision_config_t *config,
                            bool irrigation_active,
                            uint64_t now_ms,
                            alarm_manager_t *alarm_manager)
{
    if (manager == 0 || measurement == 0 || config == 0 || alarm_manager == 0) {
        return false;
    }

    uint32_t max_age_ms = irrigation_active ? config->active_max_age_ms : config->idle_max_age_ms;
    if (!measurement->valid || measurement->measurement_age_ms > max_age_ms) {
        manager->measurement_valid = false;
        manager->low_flow_since_ms = 0;
        manager->high_flow_since_ms = 0;
        manager->unexpected_flow_since_ms = 0;
        return false;
    }

    manager->current_lpm_x100 = measurement->flow_lpm_x100;
    manager->source = measurement->source;
    manager->measurement_age_ms = measurement->measurement_age_ms;
    manager->volume_total_ml = measurement->volume_total_ml;
    manager->pulse_count_total = measurement->pulse_count_total;
    manager->measurement_valid = true;

    if (!irrigation_active) {
        manager->low_flow_since_ms = 0;
        manager->high_flow_since_ms = 0;
        alarm_manager_clear(alarm_manager, ZIC_ALARM_LOW_FLOW);
        alarm_manager_clear(alarm_manager, ZIC_ALARM_HIGH_FLOW);

        if (measurement->flow_lpm_x100 >= config->low_flow_lpm_x100) {
            if (manager->unexpected_flow_since_ms == 0) {
                manager->unexpected_flow_since_ms = now_ms;
            }
            if (duration_elapsed(manager->unexpected_flow_since_ms, now_ms,
                                 config->high_flow_duration_ms)) {
                alarm_manager_raise(alarm_manager, ZIC_ALARM_LEAK_DETECTED, ZIC_ALARM_CRITICAL);
            }
        } else {
            manager->unexpected_flow_since_ms = 0;
            alarm_manager_clear(alarm_manager, ZIC_ALARM_LEAK_DETECTED);
        }
        return true;
    }

    manager->unexpected_flow_since_ms = 0;
    alarm_manager_clear(alarm_manager, ZIC_ALARM_LEAK_DETECTED);

    if (measurement->flow_lpm_x100 < config->low_flow_lpm_x100) {
        if (manager->low_flow_since_ms == 0) {
            manager->low_flow_since_ms = now_ms;
        }
        if (duration_elapsed(manager->low_flow_since_ms, now_ms, config->no_flow_timeout_ms)) {
            alarm_manager_raise(alarm_manager, ZIC_ALARM_LOW_FLOW, ZIC_ALARM_CRITICAL);
        }
    } else {
        manager->low_flow_since_ms = 0;
        alarm_manager_clear(alarm_manager, ZIC_ALARM_LOW_FLOW);
    }

    if (measurement->flow_lpm_x100 > config->high_flow_lpm_x100) {
        if (manager->high_flow_since_ms == 0) {
            manager->high_flow_since_ms = now_ms;
        }
        if (duration_elapsed(manager->high_flow_since_ms, now_ms,
                             config->high_flow_duration_ms)) {
            alarm_manager_raise(alarm_manager, ZIC_ALARM_HIGH_FLOW, ZIC_ALARM_CRITICAL);
        }
    } else {
        manager->high_flow_since_ms = 0;
        alarm_manager_clear(alarm_manager, ZIC_ALARM_HIGH_FLOW);
    }

    return true;
}
