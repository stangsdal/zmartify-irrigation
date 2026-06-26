/**
 * @file alarm_manager.h
 * @brief Alarm Manager - centralised alarm lifecycle and escalation
 *
 * Singleton. All components call alarm_raise() / alarm_clear() directly.
 * Architecture ref: MEP v5.0 Volume 2, Chapter 12
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>

typedef enum {
    ALARM_NONE = 0,
    ALARM_HIGH_FLOW = 1, ALARM_LOW_FLOW = 2, ALARM_NO_FLOW = 3,
    ALARM_LEAK_DETECTED = 4, ALARM_PIPE_BURST = 5,
    ALARM_HIGH_PRESSURE = 6, ALARM_LOW_PRESSURE = 7, ALARM_PRESSURE_COLLAPSE = 8,
    ALARM_RELAY_FAULT = 9, ALARM_SENSOR_FAULT = 10, ALARM_I2C_BUS_FAULT = 11,
    ALARM_IRRIGATION_FAULT = 12, ALARM_EMERGENCY_STOP = 13, ALARM_WATCHDOG_RESET = 14,
    ALARM_MQTT_DISCONNECTED = 20, ALARM_WIFI_DISCONNECTED = 21,
    ALARM_NTP_SYNC_FAILED = 22, ALARM_WEATHER_API_FAILED = 23,
    ALARM_CABINET_HOT_WARN = 30, ALARM_CABINET_HOT_CRIT = 31,
    ALARM_COUNT
} alarm_code_t;

typedef enum {
    ALARM_SEV_INFO = 0,
    ALARM_SEV_WARNING = 1,
    ALARM_SEV_CRITICAL = 2,
} alarm_severity_t;

typedef struct {
    alarm_code_t     code;
    alarm_severity_t severity;
    bool             active;
    bool             acknowledged;
    uint32_t         raised_epoch;
    uint32_t         ack_epoch;
    uint8_t          zone_id;
    uint32_t         raise_count;
} alarm_record_t;

bool alarm_manager_init(void);
void alarm_raise(alarm_code_t code, alarm_severity_t severity, uint8_t zone_id);
void alarm_clear(alarm_code_t code);
void alarm_acknowledge(alarm_code_t code);
bool alarm_is_active(alarm_code_t code);
bool alarm_get_record(alarm_code_t code, alarm_record_t *out);
uint8_t alarm_active_count(void);
uint8_t alarm_active_count_by_severity(alarm_severity_t min_severity);

/* Legacy compat: keep old API callable for existing code */
typedef struct { int _unused; } alarm_manager_t;
static inline void alarm_manager_init_legacy(alarm_manager_t *m) { (void)m; }
static inline bool alarm_manager_is_active(const alarm_manager_t *m, int code)
{ (void)m; return alarm_is_active((alarm_code_t)code); }
