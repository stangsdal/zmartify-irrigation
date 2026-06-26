/*
 * storage_manager.c - Event Logger v5.0
 */
#include "storage_manager.h"
#include "event_bus.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "storage_mgr";

static log_entry_t       s_buf[LOG_BUFFER_SIZE];
static size_t            s_head  = 0;
static size_t            s_count = 0;
static SemaphoreHandle_t s_lock  = NULL;
static bool              s_initialized = false;

static uint32_t current_epoch(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000000ULL);
}

static void on_event(const zic_event_t *ev, void *ctx)
{
    (void)ctx;
    log_category_t cat = LOG_CAT_SYSTEM;
    uint8_t sev = 0;
    char msg[64] = {0};

    switch ((event_id_e)ev->event_id)
    {
        case EVENT_ZONE_STARTED:
            cat = LOG_CAT_IRRIGATION; sev = 0;
            snprintf(msg, sizeof(msg), "Zone %u started",
                     ev->payload_size > 0 ? ev->payload[0] : 0u);
            break;
        case EVENT_ZONE_STOPPED:
            cat = LOG_CAT_IRRIGATION; sev = 0;
            snprintf(msg, sizeof(msg), "Zone %u stopped",
                     ev->payload_size > 0 ? ev->payload[0] : 0u);
            break;
        case EVENT_ZONE_FAULT:
            cat = LOG_CAT_IRRIGATION; sev = 2;
            snprintf(msg, sizeof(msg), "Zone %u fault",
                     ev->payload_size > 0 ? ev->payload[0] : 0u);
            break;
        case EVENT_IRRIGATION_STARTED:
            cat = LOG_CAT_IRRIGATION; sev = 0;
            snprintf(msg, sizeof(msg), "Irrigation started");
            break;
        case EVENT_IRRIGATION_COMPLETED:
            cat = LOG_CAT_IRRIGATION; sev = 0;
            snprintf(msg, sizeof(msg), "Irrigation completed");
            break;
        case EVENT_IRRIGATION_FAULT:
            cat = LOG_CAT_IRRIGATION; sev = 2;
            snprintf(msg, sizeof(msg), "Irrigation fault");
            break;
        case EVENT_ALARM_GENERATED:
            cat = LOG_CAT_ALARM;
            sev = ev->payload_size > 1 ? ev->payload[1] : 1u;
            snprintf(msg, sizeof(msg), "Alarm %u sev=%u",
                     ev->payload_size > 0 ? ev->payload[0] : 0u, sev);
            break;
        case EVENT_ALARM_RESOLVED:
            cat = LOG_CAT_ALARM; sev = 0;
            snprintf(msg, sizeof(msg), "Alarm resolved");
            break;
        case EVENT_FLOW_ANOMALY:
            cat = LOG_CAT_HYDRAULIC; sev = 2;
            snprintf(msg, sizeof(msg), "Flow anomaly");
            break;
        case EVENT_PRESSURE_OUT_OF_BOUNDS:
            cat = LOG_CAT_HYDRAULIC; sev = 2;
            snprintf(msg, sizeof(msg), "Pressure out of bounds");
            break;
        case EVENT_WEATHER_UPDATED:
            cat = LOG_CAT_WEATHER; sev = 0;
            snprintf(msg, sizeof(msg), "Weather updated");
            break;
        case EVENT_MQTT_CONNECTED:
            cat = LOG_CAT_NETWORK; sev = 0;
            snprintf(msg, sizeof(msg), "MQTT connected");
            break;
        case EVENT_MQTT_DISCONNECTED:
            cat = LOG_CAT_NETWORK; sev = 1;
            snprintf(msg, sizeof(msg), "MQTT disconnected");
            break;
        case EVENT_SYSTEM_BOOT:
            cat = LOG_CAT_SYSTEM; sev = 0;
            snprintf(msg, sizeof(msg), "System boot");
            break;
        default:
            return;
    }
    storage_manager_log(cat, sev, 0, (uint32_t)ev->event_id, msg);
}

bool storage_manager_init(void)
{
    if (s_initialized) return true;

    s_lock = xSemaphoreCreateMutex();
    if (!s_lock) return false;

    memset(s_buf, 0, sizeof(s_buf));
    s_head = s_count = 0;

    event_bus_subscribe(0xFFFFFFFF, on_event, NULL);

    s_initialized = true;
    ESP_LOGI(TAG, "Storage Manager initialized (%d entries)", LOG_BUFFER_SIZE);
    return true;
}

void storage_manager_log(log_category_t cat, uint8_t severity, uint8_t zone_id,
                          uint32_t value, const char *message)
{
    if (!s_initialized) return;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    log_entry_t *e = &s_buf[s_head % LOG_BUFFER_SIZE];
    e->timestamp_epoch = current_epoch();
    e->category  = cat;
    e->severity  = severity;
    e->zone_id   = zone_id;
    e->value     = value;
    if (message) {
        strncpy(e->message, message, LOG_MSG_MAX - 1);
        e->message[LOG_MSG_MAX - 1] = 0;
    } else {
        e->message[0] = 0;
    }
    s_head++;
    if (s_count < LOG_BUFFER_SIZE) s_count++;
    xSemaphoreGive(s_lock);
}

size_t storage_manager_count(void)  { return s_count; }

bool storage_manager_get(size_t index, log_entry_t *out)
{
    if (!out || index >= s_count) return false;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    size_t real_idx = (s_head - s_count + index) % LOG_BUFFER_SIZE;
    *out = s_buf[real_idx];
    xSemaphoreGive(s_lock);
    return true;
}

size_t storage_manager_export_json(char *buf, size_t buf_len, size_t max_entries)
{
    if (!buf || buf_len < 4) return 0;
    xSemaphoreTake(s_lock, portMAX_DELAY);
    size_t n = (max_entries && max_entries < s_count) ? max_entries : s_count;
    xSemaphoreGive(s_lock);

    size_t w = 0;
    w += (size_t)snprintf(buf + w, buf_len - w, "[");
    for (size_t i = 0; i < n && w < buf_len - 2; i++) {
        log_entry_t e;
        if (!storage_manager_get(i, &e)) continue;
        if (i > 0 && w < buf_len - 2) buf[w++] = ',';
        int r = snprintf(buf + w, buf_len - w,
                 "{\"t\":%lu,\"cat\":%u,\"sev\":%u,\"z\":%u,\"v\":%lu,\"msg\":\"%s\"}",
                 (unsigned long)e.timestamp_epoch, (unsigned)e.category,
                 (unsigned)e.severity, (unsigned)e.zone_id,
                 (unsigned long)e.value, e.message);
        if (r > 0) w += (size_t)r;
    }
    if (w < buf_len - 1) { buf[w++] = ']'; buf[w] = 0; }
    return w;
}

void storage_manager_clear(void)
{
    xSemaphoreTake(s_lock, portMAX_DELAY);
    s_head = s_count = 0;
    xSemaphoreGive(s_lock);
    ESP_LOGI(TAG, "Log cleared");
}
