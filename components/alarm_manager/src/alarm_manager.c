#include "alarm_manager.h"

#include <stddef.h>

static alarm_manager_t s_legacy_manager;
static bool s_legacy_initialized;

static alarm_manager_t *legacy_manager(void)
{
    if (!s_legacy_initialized) {
        alarm_manager_init(&s_legacy_manager);
        s_legacy_initialized = true;
    }

    return &s_legacy_manager;
}

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

void alarm_raise(zic_alarm_code_t code, zic_alarm_severity_t severity, uint8_t zone_id)
{
    (void)zone_id;
    alarm_manager_raise(legacy_manager(), code, severity);
}

uint8_t alarm_active_count(void)
{
    uint8_t count = 0;
    const alarm_manager_t *manager = legacy_manager();

    for (int i = 0; i < ZIC_MAX_ACTIVE_ALARMS; ++i) {
        if (manager->alarms[i].active) {
            ++count;
        }
    }

    return count;
}

uint8_t alarm_active_count_by_severity(zic_alarm_severity_t severity)
{
    uint8_t count = 0;
    const alarm_manager_t *manager = legacy_manager();

    for (int i = 0; i < ZIC_MAX_ACTIVE_ALARMS; ++i) {
        if (manager->alarms[i].active && manager->alarms[i].severity == severity) {
            ++count;
        }
    }

    return count;
}
