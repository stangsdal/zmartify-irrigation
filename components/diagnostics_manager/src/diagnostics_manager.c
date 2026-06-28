/*
 * diagnostics_manager.c
 * Diagnostics Manager v5.0 – health monitoring and OTA rollback guard
 */

#include "diagnostics_manager.h"
#include "alarm_manager.h"
#include "storage_manager.h"
#include "event_bus.h"
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
#define HEAP_CRITICAL_PCT     90u     /**< Heap utilisation above this = unhealthy  */

static bool s_initialized = false;

static bool ota_rollback_supported(void)
{
#if defined(CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE) && CONFIG_BOOTLOADER_APP_ROLLBACK_ENABLE
    const esp_partition_t *running = esp_ota_get_running_partition();
    const esp_partition_t *otadata = esp_partition_find_first(
        ESP_PARTITION_TYPE_DATA,
        ESP_PARTITION_SUBTYPE_DATA_OTA,
        NULL);

    if (running == NULL || otadata == NULL)
    {
        return false;
    }

    return running->subtype >= ESP_PARTITION_SUBTYPE_APP_OTA_0
        && running->subtype < ESP_PARTITION_SUBTYPE_APP_OTA_MAX;
#else
    return false;
#endif
}

/* ─── OTA rollback guard task ─────────────────────────────────────────── */

static void ota_health_check_task(void *arg)
{
    (void)arg;

    ESP_LOGI(TAG, "OTA health check: waiting %u ms before confirming...",
             OTA_CONFIRM_DELAY_MS);

    vTaskDelay(pdMS_TO_TICKS(OTA_CONFIRM_DELAY_MS));

    if (diagnostics_is_healthy())
    {
        esp_err_t err = esp_ota_mark_app_valid_cancel_rollback();
        if (err == ESP_OK)
        {
            ESP_LOGI(TAG, "OTA firmware confirmed valid – rollback cancelled");
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
        alarm_raise(ALARM_IRRIGATION_FAULT, ALARM_SEV_CRITICAL, 0);

        if (ota_rollback_supported())
        {
            ESP_LOGE(TAG, "Health check FAILED - triggering OTA rollback");
            esp_ota_mark_app_invalid_rollback_and_reboot();
        }
        else
        {
            ESP_LOGW(TAG, "Health check FAILED - rollback unavailable (single-app or rollback disabled)");
        }
    }

    vTaskDelete(NULL);
}

/* ─── Public API ──────────────────────────────────────────────────────── */

bool diagnostics_manager_init(void)
{
    if (s_initialized)
    {
        return true;
    }

    /* Start OTA rollback guard task at lowest priority */
    xTaskCreate(ota_health_check_task, "ota_guard", 3072, NULL, 1, NULL);

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

    /* Alarms */
    out->active_alarms   = alarm_active_count();
    out->critical_alarms = alarm_active_count_by_severity(ALARM_SEV_CRITICAL);

    /* Event bus */
    event_bus_stats_t stats;
    if (event_bus_get_stats(&stats))
    {
        out->events_processed = stats.events_processed;
        out->events_dropped   = stats.events_dropped;
    }

    /* Misc */
    out->reset_reason    = (uint8_t)esp_reset_reason();
    out->log_entries     = (uint32_t)storage_manager_count();
    out->subsystems_ready = (out->critical_alarms == 0);

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
             "\"healthy\":%s"
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
             h.subsystems_ready && h.heap_utilisation_pct < HEAP_CRITICAL_PCT
                ? "true" : "false");

    return (r > 0 && (size_t)r < len) ? (size_t)r : len - 1;
}

bool diagnostics_is_healthy(void)
{
    diag_health_t h;
    if (!diagnostics_get_health(&h))
    {
        return false;
    }
    return h.critical_alarms == 0
        && h.heap_utilisation_pct < HEAP_CRITICAL_PCT
        && h.subsystems_ready;
}
