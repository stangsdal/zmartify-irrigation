#include "pressure_manager.h"

void pressure_manager_init(pressure_manager_t *manager,
                           uint32_t min_pressure_mbar,
                           uint32_t max_pressure_mbar)
{
    if (manager == 0) {
        return;
    }

    manager->min_pressure_mbar = min_pressure_mbar;
    manager->max_pressure_mbar = max_pressure_mbar;
    manager->current_pressure_mbar = 0;
    manager->low_pressure_since_ms = 0;
    manager->high_pressure_since_ms = 0;
    manager->measurement_valid = false;
}

void pressure_manager_update(pressure_manager_t *manager,
                             uint32_t current_pressure_mbar,
                             alarm_manager_t *alarm_manager)
{
    if (manager == 0 || alarm_manager == 0) {
        return;
    }

    manager->current_pressure_mbar = current_pressure_mbar;

    if (current_pressure_mbar < manager->min_pressure_mbar) {
        alarm_manager_raise(alarm_manager, ZIC_ALARM_LOW_PRESSURE, ZIC_ALARM_WARNING);
        return;
    }

    if (current_pressure_mbar > manager->max_pressure_mbar) {
        alarm_manager_raise(alarm_manager, ZIC_ALARM_HIGH_PRESSURE, ZIC_ALARM_WARNING);
        return;
    }

    alarm_manager_clear(alarm_manager, ZIC_ALARM_LOW_PRESSURE);
    alarm_manager_clear(alarm_manager, ZIC_ALARM_HIGH_PRESSURE);
    alarm_manager_clear(alarm_manager, ZIC_ALARM_PRESSURE_COLLAPSE);
}

bool pressure_manager_supervise(pressure_manager_t *manager,
                                const pressure_supervision_config_t *config,
                                bool measurement_valid,
                                uint32_t current_pressure_mbar,
                                bool irrigation_active,
                                uint64_t now_ms,
                                alarm_manager_t *alarm_manager)
{
    if (manager == 0 || config == 0 || alarm_manager == 0) {
        return false;
    }

    manager->measurement_valid = measurement_valid;
    if (!measurement_valid || !irrigation_active) {
        manager->low_pressure_since_ms = 0;
        manager->high_pressure_since_ms = 0;
        pressure_manager_clear_alarms(alarm_manager);
        return measurement_valid;
    }

    manager->current_pressure_mbar = current_pressure_mbar;
    if (current_pressure_mbar < config->low_pressure_mbar) {
        if (manager->low_pressure_since_ms == 0) {
            manager->low_pressure_since_ms = now_ms;
        }
        alarm_manager_raise(alarm_manager, ZIC_ALARM_LOW_PRESSURE, ZIC_ALARM_WARNING);
        if (now_ms - manager->low_pressure_since_ms >= config->critical_duration_ms) {
            alarm_manager_raise(alarm_manager, ZIC_ALARM_PRESSURE_COLLAPSE, ZIC_ALARM_CRITICAL);
        }
    } else {
        manager->low_pressure_since_ms = 0;
        alarm_manager_clear(alarm_manager, ZIC_ALARM_LOW_PRESSURE);
        alarm_manager_clear(alarm_manager, ZIC_ALARM_PRESSURE_COLLAPSE);
    }

    if (current_pressure_mbar > config->high_pressure_mbar) {
        if (manager->high_pressure_since_ms == 0) {
            manager->high_pressure_since_ms = now_ms;
        }
        zic_alarm_severity_t severity =
            now_ms - manager->high_pressure_since_ms >= config->critical_duration_ms
                ? ZIC_ALARM_CRITICAL
                : ZIC_ALARM_WARNING;
        alarm_manager_raise(alarm_manager, ZIC_ALARM_HIGH_PRESSURE, severity);
    } else {
        manager->high_pressure_since_ms = 0;
        alarm_manager_clear(alarm_manager, ZIC_ALARM_HIGH_PRESSURE);
    }
    return true;
}

void pressure_manager_clear_alarms(alarm_manager_t *alarm_manager)
{
    if (alarm_manager == 0) {
        return;
    }
    alarm_manager_clear(alarm_manager, ZIC_ALARM_LOW_PRESSURE);
    alarm_manager_clear(alarm_manager, ZIC_ALARM_HIGH_PRESSURE);
}
