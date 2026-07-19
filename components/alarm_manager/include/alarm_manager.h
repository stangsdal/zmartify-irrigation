#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ZIC_MAX_ACTIVE_ALARMS 16
#define ZIC_ALARM_HISTORY_CAPACITY 32
#define ZIC_ALARM_SNAPSHOT_MAGIC 0x5A414C4Du
#define ZIC_ALARM_SNAPSHOT_VERSION 1u

typedef enum {
    ZIC_ALARM_INFO = 0,
    ZIC_ALARM_WARNING,
    ZIC_ALARM_CRITICAL
} zic_alarm_severity_t;

typedef enum {
    ZIC_ALARM_NONE = 0,
    ZIC_ALARM_IRRIGATION_FAULT,
    ZIC_ALARM_HIGH_FLOW,
    ZIC_ALARM_LOW_FLOW,
    ZIC_ALARM_LEAK_DETECTED,
    ZIC_ALARM_PIPE_BREAK,
    ZIC_ALARM_HIGH_PRESSURE,
    ZIC_ALARM_LOW_PRESSURE,
    ZIC_ALARM_PRESSURE_COLLAPSE,
    ZIC_ALARM_VALVE_DIAGNOSTICS_UNAVAILABLE,
    ZIC_ALARM_VALVE_NO_RESPONSE,
    ZIC_ALARM_VALVE_LIKELY_STUCK_CLOSED,
    ZIC_ALARM_VALVE_LIKELY_STUCK_OPEN
} zic_alarm_code_t;

typedef enum {
    ZIC_ALARM_STATE_CLEARED = 0,
    ZIC_ALARM_STATE_ACTIVE,
    ZIC_ALARM_STATE_ACKNOWLEDGED,
    ZIC_ALARM_STATE_RESOLVED
} zic_alarm_state_t;

typedef enum {
    ZIC_ALARM_RECOVERY_AUTOMATIC = 0,
    ZIC_ALARM_RECOVERY_MANUAL_CLEAR
} zic_alarm_recovery_policy_t;

typedef struct {
    zic_alarm_code_t code;
    zic_alarm_severity_t severity;
    zic_alarm_state_t state;
    bool acknowledged;
    bool active;
} zic_alarm_t;

typedef struct {
    zic_alarm_code_t code;
    zic_alarm_severity_t severity;
    zic_alarm_state_t from;
    zic_alarm_state_t to;
} zic_alarm_transition_t;

typedef struct {
    zic_alarm_t alarms[ZIC_MAX_ACTIVE_ALARMS];
    zic_alarm_transition_t history[ZIC_ALARM_HISTORY_CAPACITY];
    uint8_t history_head;
    uint8_t history_count;
    bool dirty;
} alarm_manager_t;

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t reserved;
    uint32_t crc32;
    zic_alarm_t alarms[ZIC_MAX_ACTIVE_ALARMS];
    zic_alarm_transition_t history[ZIC_ALARM_HISTORY_CAPACITY];
    uint8_t history_head;
    uint8_t history_count;
    uint8_t padding[2];
} alarm_manager_snapshot_t;

void alarm_manager_init(alarm_manager_t *manager);
void alarm_manager_raise(alarm_manager_t *manager, zic_alarm_code_t code, zic_alarm_severity_t severity);
void alarm_manager_clear(alarm_manager_t *manager, zic_alarm_code_t code);
bool alarm_manager_acknowledge(alarm_manager_t *manager, zic_alarm_code_t code);
bool alarm_manager_manual_clear(alarm_manager_t *manager, zic_alarm_code_t code);
bool alarm_manager_is_active(const alarm_manager_t *manager, zic_alarm_code_t code);
bool alarm_manager_has_severity(const alarm_manager_t *manager, zic_alarm_severity_t severity);
bool alarm_manager_has_lockout(const alarm_manager_t *manager);
zic_alarm_state_t alarm_manager_get_state(const alarm_manager_t *manager,
                                          zic_alarm_code_t code);
zic_alarm_recovery_policy_t alarm_manager_recovery_policy(zic_alarm_code_t code,
                                                           zic_alarm_severity_t severity);
size_t alarm_manager_history_count(const alarm_manager_t *manager);
bool alarm_manager_history_get(const alarm_manager_t *manager,
                               size_t chronological_index,
                               zic_alarm_transition_t *out);
bool alarm_manager_export_snapshot(const alarm_manager_t *manager,
                                   alarm_manager_snapshot_t *snapshot);
bool alarm_manager_restore_snapshot(alarm_manager_t *manager,
                                    const alarm_manager_snapshot_t *snapshot);
bool alarm_manager_is_dirty(const alarm_manager_t *manager);
void alarm_manager_mark_persisted(alarm_manager_t *manager);

/* Legacy compatibility aliases used by older components. */
#define ALARM_SEV_INFO ZIC_ALARM_INFO
#define ALARM_SEV_WARNING ZIC_ALARM_WARNING
#define ALARM_SEV_CRITICAL ZIC_ALARM_CRITICAL
#define ALARM_IRRIGATION_FAULT ZIC_ALARM_IRRIGATION_FAULT

void alarm_raise(zic_alarm_code_t code, zic_alarm_severity_t severity, uint8_t zone_id);
uint8_t alarm_active_count(void);
uint8_t alarm_active_count_by_severity(zic_alarm_severity_t severity);
