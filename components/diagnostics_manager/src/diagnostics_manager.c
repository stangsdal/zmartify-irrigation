/*
 * diagnostics_manager.c
 * Diagnostics Manager v5.0 – health monitoring and OTA rollback guard
 */

#include "diagnostics_manager.h"
#include "event_bus.h"
#include "ota_manager.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_heap_caps.h"
#include "esp_ota_ops.h"
#include "esp_partition.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "diag_mgr";

#define OTA_CONFIRM_DELAY_MS  30000u  /**< Wait this long before marking OTA valid */
#define OTA_GUARD_TASK_STACK  16384u  /**< Includes RSA verification of rollback candidate */
static bool s_initialized = false;
static diagnostics_manager_config_t s_config;

/* ─── OTA rollback guard task ─────────────────────────────────────────── */

static void ota_health_check_task(void *arg)
{
    (void)arg;

    const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t image_state = ESP_OTA_IMG_UNDEFINED;
    if (running == NULL || esp_ota_get_state_partition(running, &image_state) != ESP_OK ||
        image_state != ESP_OTA_IMG_PENDING_VERIFY)
    {
        ESP_LOGD(TAG, "Running image does not require OTA confirmation");
        vTaskDelete(NULL);
        return;
    }

    if (ota_manager_get_state() != OTA_STATE_PENDING_CONFIRMATION)
    {
        (void)ota_manager_transition(OTA_STATE_PENDING_CONFIRMATION);
    }
    if (s_config.audit != NULL)
    {
        s_config.audit(s_config.context, "ota pending health confirmation");
    }

    ESP_LOGI(TAG, "OTA health check: waiting %u ms before confirming...",
             OTA_CONFIRM_DELAY_MS);

    vTaskDelay(pdMS_TO_TICKS(OTA_CONFIRM_DELAY_MS));

    if (diagnostics_is_healthy())
    {
        esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
        if (err == ESP_OK)
        {
            (void)ota_manager_transition(OTA_STATE_VALID);
            if (s_config.audit != NULL)
            {
                s_config.audit(s_config.context, "ota image confirmed valid");
            }
            if (s_config.ota_confirmed != NULL)
            {
                s_config.ota_confirmed(s_config.context);
            }
            (void)event_bus_publish(EVENT_OTA_COMPLETE, 0, EVENT_PRIORITY_HIGH, 0, NULL, 0);
            ESP_LOGI(TAG, "OTA firmware confirmed valid; rollback cancelled");
        }
        else if (err == ESP_ERR_OTA_VALIDATE_FAILED)
        {
            /* Not an OTA boot (e.g. factory partition) – this is fine */
            ESP_LOGD(TAG, "Not an OTA boot, no confirmation needed");
        }
        else
        {
            ESP_LOGW(TAG, "ota_mark_valid returned: %d", err);
        }
    }
    else
    {
        (void)ota_manager_transition(OTA_STATE_ROLLING_BACK);
        esp_err_t rollback_err = esp_ota_mark_app_invalid_rollback();
        if (rollback_err != ESP_OK)
        {
            ESP_LOGE(TAG, "OTA rollback failed: %s", esp_err_to_name(rollback_err));
            (void)ota_manager_transition(OTA_STATE_FAILED);
            vTaskDelete(NULL);
            return;
        }
        if (s_config.raise_critical_alarm != NULL)
        {
            s_config.raise_critical_alarm(s_config.context);
        }
        if (s_config.audit != NULL)
        {
            s_config.audit(s_config.context, "ota health failed; rollback requested");
        }
        (void)event_bus_publish(EVENT_OTA_ROLLBACK, 0, EVENT_PRIORITY_CRITICAL, 0, NULL, 0);
        ESP_LOGE(TAG, "Health check failed; triggering OTA rollback");
        esp_restart();
    }

    vTaskDelete(NULL);
}

/* ─── Public API ──────────────────────────────────────────────────────── */

bool diagnostics_manager_init(const diagnostics_manager_config_t *config)
{
    if (s_initialized)
    {
        return true;
    }

    if (config == NULL || config->snapshot == NULL)
    {
        return false;
    }

    s_config = *config;
    if (xTaskCreate(ota_health_check_task, "ota_guard", OTA_GUARD_TASK_STACK,
                    NULL, 1, NULL) != pdPASS)
    {
        memset(&s_config, 0, sizeof(s_config));
        return false;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Diagnostics Manager initialized (OTA guard active)");
    return true;
}

bool diagnostics_get_health(diag_health_t *out)
{
    if (!out)
    {
        return false;
    }

    memset(out, 0, sizeof(*out));

    /* Timing */
    out->uptime_s = (uint32_t)(esp_timer_get_time() / 1000000ULL);

    /* Heap */
    out->heap_free_bytes  = (uint32_t)heap_caps_get_free_size(MALLOC_CAP_DEFAULT);
    out->heap_min_bytes   = (uint32_t)heap_caps_get_minimum_free_size(MALLOC_CAP_DEFAULT);
    out->heap_total_bytes = (uint32_t)heap_caps_get_total_size(MALLOC_CAP_DEFAULT);
    if (out->heap_total_bytes > 0)
    {
        uint32_t used = out->heap_total_bytes - out->heap_free_bytes;
        out->heap_utilisation_pct = (uint8_t)((used * 100u) / out->heap_total_bytes);
    }

    /* Event bus */
    event_bus_stats_t stats;
    if (event_bus_get_stats(&stats))
    {
        out->events_processed = stats.events_processed;
        out->events_dropped   = stats.events_dropped;
    }

    diag_policy_input_t policy_input = {
        .heap_utilisation_pct = out->heap_utilisation_pct,
        .event_drops = out->events_dropped,
    };
    if (!s_config.snapshot(s_config.context, &policy_input,
                           &out->active_alarms, &out->log_entries))
    {
        return false;
    }
    out->critical_alarms = policy_input.critical_alarms;
    out->flow_sensor_available = policy_input.flow_sensor_available;
    out->pressure_sensor_available = policy_input.pressure_sensor_available;
    out->mqtt_connected = policy_input.mqtt_connected;
    out->time_synchronized = policy_input.time_synchronized;
    out->storage_ready = policy_input.storage_ready;
    out->storage_last_write_ok = policy_input.storage_last_write_ok;
    out->control_stack_free_bytes = policy_input.control_stack_free_bytes;
    out->telemetry_stack_free_bytes = policy_input.telemetry_stack_free_bytes;
    out->watersensor_stack_free_bytes = policy_input.watersensor_stack_free_bytes;
    out->status = diagnostics_policy_evaluate(&policy_input);

    /* Misc */
    out->reset_reason    = (uint8_t)esp_reset_reason();
    return true;
}

size_t diagnostics_health_to_json(char *buf, size_t len)
{
    if (!buf || len < 64)
    {
        return 0;
    }

    diag_health_t h;
    if (!diagnostics_get_health(&h))
    {
        return 0;
    }

    int r = snprintf(buf, len,
             "{"
             "\"uptime_s\":%lu,"
             "\"heap_free\":%lu,"
             "\"heap_min\":%lu,"
             "\"heap_pct\":%u,"
             "\"active_alarms\":%u,"
             "\"critical_alarms\":%u,"
             "\"events_processed\":%lu,"
             "\"events_dropped\":%lu,"
             "\"reset_reason\":%u,"
             "\"log_entries\":%lu,"
             "\"overall\":\"%s\","
             "\"runtime\":\"%s\","
             "\"hydraulics\":\"%s\","
             "\"communications\":\"%s\","
             "\"storage\":\"%s\","
             "\"flow_available\":%s,"
             "\"pressure_available\":%s,"
             "\"mqtt_connected\":%s,"
             "\"time_synchronized\":%s,"
             "\"control_stack_free\":%lu,"
             "\"telemetry_stack_free\":%lu,"
             "\"watersensor_stack_free\":%lu,"
             "\"ota_acceptable\":%s"
             "}",
             (unsigned long)h.uptime_s,
             (unsigned long)h.heap_free_bytes,
             (unsigned long)h.heap_min_bytes,
             (unsigned)h.heap_utilisation_pct,
             (unsigned)h.active_alarms,
             (unsigned)h.critical_alarms,
             (unsigned long)h.events_processed,
             (unsigned long)h.events_dropped,
             (unsigned)h.reset_reason,
             (unsigned long)h.log_entries,
                 diagnostics_status_name(h.status.overall),
                 diagnostics_status_name(h.status.runtime),
                 diagnostics_status_name(h.status.hydraulics),
                 diagnostics_status_name(h.status.communications),
                 diagnostics_status_name(h.status.storage),
                 h.flow_sensor_available ? "true" : "false",
                 h.pressure_sensor_available ? "true" : "false",
                 h.mqtt_connected ? "true" : "false",
                 h.time_synchronized ? "true" : "false",
                 (unsigned long)h.control_stack_free_bytes,
                 (unsigned long)h.telemetry_stack_free_bytes,
                 (unsigned long)h.watersensor_stack_free_bytes,
                 h.status.ota_acceptable ? "true" : "false");

    return (r > 0 && (size_t)r < len) ? (size_t)r : 0u;
}

bool diagnostics_is_healthy(void)
{
    diag_health_t h;
    if (!diagnostics_get_health(&h))
    {
        return false;
    }
    return h.status.ota_acceptable;
}
