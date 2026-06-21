#include "alarm_manager.h"

#include <stddef.h>

void alarm_manager_init(alarm_manager_t *manager)
{
    if (manager == NULL) {
        return;
    }

    for (int i = 0; i < ZIC_MAX_ACTIVE_ALARMS; ++i) {
        manager->alarms[i].code = ZIC_ALARM_NONE;
        manager->alarms[i].severity = ZIC_ALARM_INFO;
        manager->alarms[i].active = false;
    }
}

void alarm_manager_raise(alarm_manager_t *manager, zic_alarm_code_t code, zic_alarm_severity_t severity)
{
    if (manager == NULL || code == ZIC_ALARM_NONE) {
        return;
    }

    for (int i = 0; i < ZIC_MAX_ACTIVE_ALARMS; ++i) {
        if (manager->alarms[i].active && manager->alarms[i].code == code) {
            manager->alarms[i].severity = severity;
            return;
        }
    }

    for (int i = 0; i < ZIC_MAX_ACTIVE_ALARMS; ++i) {
        if (!manager->alarms[i].active) {
            manager->alarms[i].code = code;
            manager->alarms[i].severity = severity;
            manager->alarms[i].active = true;
            return;
        }
    }
}

void alarm_manager_clear(alarm_manager_t *manager, zic_alarm_code_t code)
{
    if (manager == NULL || code == ZIC_ALARM_NONE) {
        return;
    }

    for (int i = 0; i < ZIC_MAX_ACTIVE_ALARMS; ++i) {
        if (manager->alarms[i].active && manager->alarms[i].code == code) {
            manager->alarms[i].active = false;
            manager->alarms[i].code = ZIC_ALARM_NONE;
            manager->alarms[i].severity = ZIC_ALARM_INFO;
            return;
        }
    }
}

bool alarm_manager_is_active(const alarm_manager_t *manager, zic_alarm_code_t code)
{
    if (manager == NULL || code == ZIC_ALARM_NONE) {
        return false;
    }

    for (int i = 0; i < ZIC_MAX_ACTIVE_ALARMS; ++i) {
        if (manager->alarms[i].active && manager->alarms[i].code == code) {
            return true;
        }
    }

    return false;
}
