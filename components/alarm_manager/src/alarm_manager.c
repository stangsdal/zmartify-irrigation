/*
 * alarm_manager.c - Alarm Manager v5.0
 */
#include "alarm_manager.h"
#include "event_bus.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "alarm_mgr";

static alarm_record_t    s_alarms[ALARM_COUNT];
static SemaphoreHandle_t s_lock        = NULL;
static bool              s_initialized = false;

static uint32_t current_epoch(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000000ULL);
}

/* Subscribe to hydraulic/system fault events and auto-raise alarms */
static void on_fault_event(const zic_event_t *ev, void *ctx)
{
    (void)ctx;
    switch (ev->event_id)
    {
        case EVENT_FLOW_ANOMALY:
            alarm_raise(ALARM_HIGH_FLOW, ALARM_SEV_CRITICAL, 0);
            break;
        case EVENT_PRESSURE_OUT_OF_BOUNDS:
            alarm_raise(ALARM_HIGH_PRESSURE, ALARM_SEV_CRITICAL, 0);
            break;
        case EVENT_IRRIGATION_FAULT:
            alarm_raise(ALARM_IRRIGATION_FAULT, ALARM_SEV_CRITICAL, 0);
            break;
        case EVENT_ZONE_FAULT:
            alarm_raise(ALARM_RELAY_FAULT, ALARM_SEV_WARNING,
                        ev->payload_size > 0 ? ev->payload[0] : 0);
            break;
        case EVENT_MQTT_DISCONNECTED:
            alarm_raise(ALARM_MQTT_DISCONNECTED, ALARM_SEV_WARNING, 0);
            break;
        case EVENT_MQTT_CONNECTED:
            alarm_clear(ALARM_MQTT_DISCONNECTED);
            break;
        default:
            break;
    }
}

bool alarm_manager_init(void)
{
    if (s_initialized) return true;

    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return false;

    memset(s_alarms, 0, sizeof(s_alarms));
    for (int i = 0; i < ALARM_COUNT; i++) s_alarms[i].code = (alarm_code_t)i;

    /* Subscribe to fault events */
    event_bus_subscribe(EVENT_FLOW_ANOMALY,           on_fault_event, NULL);
    event_bus_subscribe(EVENT_PRESSURE_OUT_OF_BOUNDS, on_fault_event, NULL);
    event_bus_subscribe(EVENT_IRRIGATION_FAULT,       on_fault_event, NULL);
    event_bus_subscribe(EVENT_ZONE_FAULT,             on_fault_event, NULL);
    event_bus_subscribe(EVENT_MQTT_DISCONNECTED,      on_fault_event, NULL);
    event_bus_subscribe(EVENT_MQTT_CONNECTED,         on_fault_event, NULL);

    s_initialized = true;
    ESP_LOGI(TAG, "Alarm Manager initialized");
    return true;
}

void alarm_raise(alarm_code_t code, alarm_severity_t severity, uint8_t zone_id)
{
    if (!s_initialized || code == ALARM_NONE || code >= ALARM_COUNT) return;

    xSemaphoreTake(s_lock, portMAX_DELAY);
    alarm_record_t *r = &s_alarms[code];
    if (!r->active)
    {
        r->active        = true;
        r->acknowledged  = false;
        r->severity      = severity;
        r->zone_id       = zone_id;
        r->raised_epoch  = current_epoch();
        r->raise_count++;
        ESP_LOGW(TAG, "ALARM %d raised (sev=%d zone=%u)", code, severity, zone_id);
    }
    else
    {
        r->raise_count++;
    }
    xSemaphoreGive(s_lock);

    /* Publish alarm event */
    uint8_t payload[3] = { (uint8_t)code, (uint8_t)severity, zone_id };
    event_bus_publish(EVENT_ALARM_GENERATED, 0,
                      (severity == ALARM_SEV_CRITICAL) ? EVENT_PRIORITY_CRITICAL
                                                       : EVENT_PRIORITY_HIGH,
                      0, payload, sizeof(payload));
}

void alarm_clear(alarm_code_t code)
{
    if (!s_initialized || code == ALARM_NONE || code >= ALARM_COUNT) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (s_alarms[code].active)
    {
        s_alarms[code].active = false;
        ESP_LOGI(TAG, "Alarm %d cleared", code);
        event_bus_publish(EVENT_ALARM_RESOLVED, 0, EVENT_PRIORITY_NORMAL, 0, NULL, 0);
    }
    xSemaphoreGive(s_lock);
}

void alarm_acknowledge(alarm_code_t code)
{
    if (!s_initialized || code >= ALARM_COUNT) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (s_alarms[code].active && !s_alarms[code].acknowledged)
    {
        s_alarms[code].acknowledged = true;
        s_alarms[code].ack_epoch    = current_epoch();
        ESP_LOGI(TAG, "Alarm %d acknowledged", code);
        event_bus_publish(EVENT_ALARM_ACKNOWLEDGED, 0, EVENT_PRIORITY_NORMAL, 0, NULL, 0);
    }
    xSemaphoreGive(s_lock);
}

bool alarm_is_active(alarm_code_t code)
{
    if (!s_initialized || code >= ALARM_COUNT) return false;
    return s_alarms[code].active;
}

bool alarm_get_record(alarm_code_t code, alarm_record_t *out)
{
    if (!s_initialized || code >= ALARM_COUNT || !out) return false;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    *out = s_alarms[code];
    xSemaphoreGive(s_lock);
    return true;
}

uint8_t alarm_active_count(void)
{
    uint8_t n = 0;
    if (!s_initialized) return 0;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    for (int i = 1; i < ALARM_COUNT; i++) if (s_alarms[i].active) n++;
    xSemaphoreGive(s_lock);
    return n;
}

uint8_t alarm_active_count_by_severity(alarm_severity_t min_sev)
{
    uint8_t n = 0;
    if (!s_initialized) return 0;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    for (int i = 1; i < ALARM_COUNT; i++)
        if (s_alarms[i].active && s_alarms[i].severity >= min_sev) n++;
    xSemaphoreGive(s_lock);
    return n;
}
