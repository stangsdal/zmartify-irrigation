#pragma once

#include <stdbool.h>
#include <stdint.h>

#define ZIC_MAX_ACTIVE_ALARMS 16

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
    ZIC_ALARM_PRESSURE_COLLAPSE
} zic_alarm_code_t;

typedef struct {
    zic_alarm_code_t code;
    zic_alarm_severity_t severity;
    bool active;
} zic_alarm_t;

typedef struct {
    zic_alarm_t alarms[ZIC_MAX_ACTIVE_ALARMS];
} alarm_manager_t;

void alarm_manager_init(alarm_manager_t *manager);
void alarm_manager_raise(alarm_manager_t *manager, zic_alarm_code_t code, zic_alarm_severity_t severity);
void alarm_manager_clear(alarm_manager_t *manager, zic_alarm_code_t code);
bool alarm_manager_is_active(const alarm_manager_t *manager, zic_alarm_code_t code);
bool alarm_manager_has_severity(const alarm_manager_t *manager, zic_alarm_severity_t severity);

/* Legacy compatibility aliases used by older components. */
#define ALARM_SEV_INFO ZIC_ALARM_INFO
#define ALARM_SEV_WARNING ZIC_ALARM_WARNING
#define ALARM_SEV_CRITICAL ZIC_ALARM_CRITICAL
#define ALARM_IRRIGATION_FAULT ZIC_ALARM_IRRIGATION_FAULT

void alarm_raise(zic_alarm_code_t code, zic_alarm_severity_t severity, uint8_t zone_id);
uint8_t alarm_active_count(void);
uint8_t alarm_active_count_by_severity(zic_alarm_severity_t severity);
