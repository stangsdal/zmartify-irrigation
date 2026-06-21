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
}
