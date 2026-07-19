#include "alarm_manager.h"

#include <stddef.h>
#include <string.h>

static alarm_manager_t s_legacy_manager;
static bool s_legacy_initialized;

static uint32_t crc32_compute(const void *data, size_t length)
{
    const uint8_t *bytes = data;
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t index = 0; index < length; ++index) {
        crc ^= bytes[index];
        for (uint8_t bit = 0; bit < 8u; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

static uint32_t snapshot_crc(const alarm_manager_snapshot_t *snapshot)
{
    const uint8_t *start = (const uint8_t *)&snapshot->alarms[0];
    return crc32_compute(start, sizeof(*snapshot) - offsetof(alarm_manager_snapshot_t, alarms));
}

static void record_transition(alarm_manager_t *manager,
                              const zic_alarm_t *alarm,
                              zic_alarm_state_t from,
                              zic_alarm_state_t to)
{
    zic_alarm_transition_t *entry = &manager->history[manager->history_head];
    entry->code = alarm->code;
    entry->severity = alarm->severity;
    entry->from = from;
    entry->to = to;
    manager->history_head = (uint8_t)((manager->history_head + 1u) %
                                      ZIC_ALARM_HISTORY_CAPACITY);
    if (manager->history_count < ZIC_ALARM_HISTORY_CAPACITY) {
        ++manager->history_count;
    }
    manager->dirty = true;
}

static zic_alarm_t *find_alarm(alarm_manager_t *manager, zic_alarm_code_t code)
{
    for (int i = 0; i < ZIC_MAX_ACTIVE_ALARMS; ++i) {
        if (manager->alarms[i].code == code &&
            manager->alarms[i].state != ZIC_ALARM_STATE_CLEARED) {
            return &manager->alarms[i];
        }
    }
    return NULL;
}

static const zic_alarm_t *find_alarm_const(const alarm_manager_t *manager,
                                           zic_alarm_code_t code)
{
    return find_alarm((alarm_manager_t *)manager, code);
}

static void release_alarm(alarm_manager_t *manager, zic_alarm_t *alarm)
{
    zic_alarm_state_t from = alarm->state;
    record_transition(manager, alarm, from, ZIC_ALARM_STATE_CLEARED);
    alarm->code = ZIC_ALARM_NONE;
    alarm->severity = ZIC_ALARM_INFO;
    alarm->state = ZIC_ALARM_STATE_CLEARED;
    alarm->acknowledged = false;
    alarm->active = false;
}

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

    *manager = (alarm_manager_t){0};
    for (int i = 0; i < ZIC_MAX_ACTIVE_ALARMS; ++i) {
        manager->alarms[i].code = ZIC_ALARM_NONE;
        manager->alarms[i].severity = ZIC_ALARM_INFO;
        manager->alarms[i].state = ZIC_ALARM_STATE_CLEARED;
        manager->alarms[i].active = false;
    }
}

zic_alarm_recovery_policy_t alarm_manager_recovery_policy(zic_alarm_code_t code,
                                                           zic_alarm_severity_t severity)
{
    switch (code) {
    case ZIC_ALARM_IRRIGATION_FAULT:
    case ZIC_ALARM_LEAK_DETECTED:
    case ZIC_ALARM_PIPE_BREAK:
    case ZIC_ALARM_PRESSURE_COLLAPSE:
    case ZIC_ALARM_VALVE_LIKELY_STUCK_CLOSED:
    case ZIC_ALARM_VALVE_LIKELY_STUCK_OPEN:
        return ZIC_ALARM_RECOVERY_MANUAL_CLEAR;
    default:
        return severity == ZIC_ALARM_CRITICAL
            ? ZIC_ALARM_RECOVERY_MANUAL_CLEAR
            : ZIC_ALARM_RECOVERY_AUTOMATIC;
    }
}

void alarm_manager_raise(alarm_manager_t *manager, zic_alarm_code_t code, zic_alarm_severity_t severity)
{
    if (manager == NULL || code == ZIC_ALARM_NONE) {
        return;
    }

    zic_alarm_t *alarm = find_alarm(manager, code);
    if (alarm != NULL) {
        if (alarm->severity < severity) {
            alarm->severity = severity;
            manager->dirty = true;
        }
        if (alarm->state == ZIC_ALARM_STATE_RESOLVED) {
            zic_alarm_state_t from = alarm->state;
            alarm->acknowledged = false;
            alarm->state = ZIC_ALARM_STATE_ACTIVE;
            alarm->active = true;
            record_transition(manager, alarm, from, alarm->state);
        }
        return;
    }

    for (int i = 0; i < ZIC_MAX_ACTIVE_ALARMS; ++i) {
        if (manager->alarms[i].state == ZIC_ALARM_STATE_CLEARED) {
            manager->alarms[i].code = code;
            manager->alarms[i].severity = severity;
            manager->alarms[i].state = ZIC_ALARM_STATE_ACTIVE;
            manager->alarms[i].acknowledged = false;
            manager->alarms[i].active = true;
            record_transition(manager, &manager->alarms[i],
                              ZIC_ALARM_STATE_CLEARED, ZIC_ALARM_STATE_ACTIVE);
            return;
        }
    }
}

void alarm_manager_clear(alarm_manager_t *manager, zic_alarm_code_t code)
{
    if (manager == NULL || code == ZIC_ALARM_NONE) {
        return;
    }

    zic_alarm_t *alarm = find_alarm(manager, code);
    if (alarm == NULL || alarm->state == ZIC_ALARM_STATE_RESOLVED) {
        return;
    }
    if (alarm_manager_recovery_policy(code, alarm->severity) ==
        ZIC_ALARM_RECOVERY_AUTOMATIC) {
        release_alarm(manager, alarm);
        return;
    }
    zic_alarm_state_t from = alarm->state;
    alarm->state = ZIC_ALARM_STATE_RESOLVED;
    alarm->active = false;
    record_transition(manager, alarm, from, alarm->state);
}

bool alarm_manager_acknowledge(alarm_manager_t *manager, zic_alarm_code_t code)
{
    if (manager == NULL || code == ZIC_ALARM_NONE) {
        return false;
    }
    zic_alarm_t *alarm = find_alarm(manager, code);
    if (alarm == NULL || alarm->acknowledged) {
        return false;
    }
    zic_alarm_state_t from = alarm->state;
    alarm->acknowledged = true;
    if (alarm->state == ZIC_ALARM_STATE_ACTIVE) {
        alarm->state = ZIC_ALARM_STATE_ACKNOWLEDGED;
    }
    record_transition(manager, alarm, from, alarm->state);
    return true;
}

bool alarm_manager_manual_clear(alarm_manager_t *manager, zic_alarm_code_t code)
{
    if (manager == NULL || code == ZIC_ALARM_NONE) {
        return false;
    }
    zic_alarm_t *alarm = find_alarm(manager, code);
    if (alarm == NULL || alarm->state != ZIC_ALARM_STATE_RESOLVED ||
        !alarm->acknowledged) {
        return false;
    }
    release_alarm(manager, alarm);
    return true;
}

bool alarm_manager_is_active(const alarm_manager_t *manager, zic_alarm_code_t code)
{
    if (manager == NULL || code == ZIC_ALARM_NONE) {
        return false;
    }

    const zic_alarm_t *alarm = find_alarm_const(manager, code);
    return alarm != NULL && alarm->active;
}

bool alarm_manager_has_severity(const alarm_manager_t *manager, zic_alarm_severity_t severity)
{
    if (manager == NULL) {
        return false;
    }
    for (int i = 0; i < ZIC_MAX_ACTIVE_ALARMS; ++i) {
        if (manager->alarms[i].state != ZIC_ALARM_STATE_CLEARED &&
            manager->alarms[i].severity == severity) {
            return true;
        }
    }
    return false;
}

bool alarm_manager_has_lockout(const alarm_manager_t *manager)
{
    return alarm_manager_has_severity(manager, ZIC_ALARM_CRITICAL);
}

zic_alarm_state_t alarm_manager_get_state(const alarm_manager_t *manager,
                                          zic_alarm_code_t code)
{
    if (manager == NULL || code == ZIC_ALARM_NONE) {
        return ZIC_ALARM_STATE_CLEARED;
    }
    const zic_alarm_t *alarm = find_alarm_const(manager, code);
    return alarm == NULL ? ZIC_ALARM_STATE_CLEARED : alarm->state;
}

size_t alarm_manager_history_count(const alarm_manager_t *manager)
{
    return manager == NULL ? 0u : manager->history_count;
}

bool alarm_manager_history_get(const alarm_manager_t *manager,
                               size_t chronological_index,
                               zic_alarm_transition_t *out)
{
    if (manager == NULL || out == NULL || chronological_index >= manager->history_count) {
        return false;
    }
    size_t oldest = (manager->history_head + ZIC_ALARM_HISTORY_CAPACITY -
                     manager->history_count) % ZIC_ALARM_HISTORY_CAPACITY;
    *out = manager->history[(oldest + chronological_index) %
                            ZIC_ALARM_HISTORY_CAPACITY];
    return true;
}

bool alarm_manager_export_snapshot(const alarm_manager_t *manager,
                                   alarm_manager_snapshot_t *snapshot)
{
    if (manager == NULL || snapshot == NULL) {
        return false;
    }
    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->magic = ZIC_ALARM_SNAPSHOT_MAGIC;
    snapshot->version = ZIC_ALARM_SNAPSHOT_VERSION;
    memcpy(snapshot->alarms, manager->alarms, sizeof(snapshot->alarms));
    memcpy(snapshot->history, manager->history, sizeof(snapshot->history));
    snapshot->history_head = manager->history_head;
    snapshot->history_count = manager->history_count;
    snapshot->crc32 = snapshot_crc(snapshot);
    return true;
}

bool alarm_manager_restore_snapshot(alarm_manager_t *manager,
                                    const alarm_manager_snapshot_t *snapshot)
{
    if (manager == NULL || snapshot == NULL ||
        snapshot->magic != ZIC_ALARM_SNAPSHOT_MAGIC ||
        snapshot->version != ZIC_ALARM_SNAPSHOT_VERSION ||
        snapshot->history_head >= ZIC_ALARM_HISTORY_CAPACITY ||
        snapshot->history_count > ZIC_ALARM_HISTORY_CAPACITY ||
        snapshot->crc32 != snapshot_crc(snapshot)) {
        return false;
    }
    for (size_t index = 0; index < ZIC_MAX_ACTIVE_ALARMS; ++index) {
        const zic_alarm_t *alarm = &snapshot->alarms[index];
        if (alarm->code > ZIC_ALARM_VALVE_LIKELY_STUCK_OPEN ||
            alarm->severity > ZIC_ALARM_CRITICAL ||
            alarm->state > ZIC_ALARM_STATE_RESOLVED ||
            (alarm->state == ZIC_ALARM_STATE_CLEARED && alarm->code != ZIC_ALARM_NONE) ||
            (alarm->state != ZIC_ALARM_STATE_CLEARED && alarm->code == ZIC_ALARM_NONE)) {
            return false;
        }
    }

    memcpy(manager->alarms, snapshot->alarms, sizeof(manager->alarms));
    memcpy(manager->history, snapshot->history, sizeof(manager->history));
    manager->history_head = snapshot->history_head;
    manager->history_count = snapshot->history_count;
    manager->dirty = false;
    return true;
}

bool alarm_manager_is_dirty(const alarm_manager_t *manager)
{
    return manager != NULL && manager->dirty;
}

void alarm_manager_mark_persisted(alarm_manager_t *manager)
{
    if (manager != NULL) {
        manager->dirty = false;
    }
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
        if (manager->alarms[i].state != ZIC_ALARM_STATE_CLEARED) {
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
        if (manager->alarms[i].state != ZIC_ALARM_STATE_CLEARED &&
            manager->alarms[i].severity == severity) {
            ++count;
        }
    }

    return count;
}
