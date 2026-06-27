/*
 * main.c
 * Zmartify Irrigation Controller - Application Entry Point
 *
 * Version 5.0 - Event-driven architecture as per Zmartify Master Engineering Package
 * Step 2: Event Bus foundation
 */

#include <stdio.h>
#include "esp_log.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "event_bus.h"
#include "hal.h"
#include "hmi_board.h"
#include "config_manager.h"
#include "relay_manager.h"
#include "zone_manager.h"
#include "irrigation_engine.h"
#include "flow_manager.h"
#include "pressure_manager.h"
#include "weather_manager.h"
#include "et_engine.h"
#include "alarm_manager.h"
#include "storage_manager.h"
#include "mqtt_manager.h"
#include "ota_manager.h"
#include "diagnostics_manager.h"
#include "version.h"

static const char *TAG = "zic_main";

/**
 * System initialization task
 */
static void system_init_task(void *arg)
{
    (void)arg;

    ESP_LOGI(TAG, "===================================");
    ESP_LOGI(TAG, "Zmartify Irrigation Controller v5.0");
    ESP_LOGI(TAG, "Firmware %s (%s %s)", ZIC_FW_VERSION_STR, ZIC_FW_BUILD_DATE, ZIC_FW_BUILD_TIME);
    ESP_LOGI(TAG, "Model: %s", ZIC_CONTROLLER_MODEL);
    ESP_LOGI(TAG, "===================================");

    // Step 1: Initialize Event Bus (foundation for all inter-module communication)
    ESP_LOGI(TAG, "[Step 2] Initializing Event Bus...");
    if (!event_bus_init())
    {
        ESP_LOGE(TAG, "Failed to initialize Event Bus!");
        vTaskDelete(NULL);
        return;
    }
    ESP_LOGI(TAG, "Event Bus initialized successfully");

    // Publish system boot event
    event_bus_publish(EVENT_SYSTEM_BOOT, 0, EVENT_PRIORITY_HIGH, 0, NULL, 0);
    vTaskDelay(pdMS_TO_TICKS(100));

    // Step 2: Initialize Hardware Abstraction Layer
    ESP_LOGI(TAG, "[Step 3] Initializing HAL...");
    if (!zic_hal_init())
    {
        ESP_LOGE(TAG, "HAL initialization failed!");
        /* Continue – some peripherals may be absent in dev */
    }
    else
    {
        ESP_LOGI(TAG, "HAL initialized successfully");
        event_bus_publish(EVENT_SYSTEM_BOOT, 0, EVENT_PRIORITY_NORMAL, 0, NULL, 0);
    }

    ESP_LOGI(TAG, "[Step 3.1] Initializing HMI board bring-up...");
    if (!hmi_board_init())
    {
        ESP_LOGW(TAG, "HMI board bring-up incomplete (continuing in headless mode)");
    }
    else
    {
        hmi_board_status_t hmi_status;
        hmi_board_get_status(&hmi_status);
        ESP_LOGI(TAG, "HMI bring-up status: backlight=%d touch=%d panel=%d",
                 hmi_status.backlight_enabled,
                 hmi_status.touch_present,
                 hmi_status.panel_ready);
    }

    // Step 3: Initialize Configuration Manager
    ESP_LOGI(TAG, "[Step 4] Initializing Configuration Manager...");
    if (config_manager_init() != CFG_OK)
    {
        ESP_LOGE(TAG, "Configuration Manager initialization failed!");
    }
    else
    {
        const zic_config_t *cfg = config_get();
        ESP_LOGI(TAG, "Config loaded: schema v%u, %u zones, mode=%d",
                 cfg->schema_version,
                 cfg->system.active_zone_count,
                 cfg->system.operational_mode);
    }

    // Step 4: Initialize Relay Manager + Zone Manager + Irrigation Engine
    ESP_LOGI(TAG, "[Step 5] Initializing Irrigation Engine...");
    {
        config_system_t sys;
        config_get_system(&sys);
        relay_manager_init((uint8_t)sys.max_simultaneous_zones);
    }
    zone_manager_init();
    if (!irrigation_engine_init())
    {
        ESP_LOGE(TAG, "Irrigation Engine initialization failed!");
    }
    else
    {
        ESP_LOGI(TAG, "Irrigation Engine ready (state: IDLE)");
    }

    // Step 5: Initialize Flow & Pressure Managers (Hydraulic Safety System)
    ESP_LOGI(TAG, "[Step 6] Initializing Hydraulic Safety System...");
    if (!flow_manager_init())
    {
        ESP_LOGW(TAG, "Flow Manager init failed (sensor may be absent)");
    }
    if (!pressure_manager_init())
    {
        ESP_LOGW(TAG, "Pressure Manager init failed (sensor may be absent)");
    }
    ESP_LOGI(TAG, "Hydraulic Safety System started");

    // Step 6: Initialize Weather Manager and ET Engine
    ESP_LOGI(TAG, "[Step 7] Initializing Weather Manager and ET Engine...");
    weather_manager_init();
    et_engine_init();
    ESP_LOGI(TAG, "Weather Manager and ET Engine ready");

    // Step 7: Initialize Alarm Manager and Storage (Event Logger)
    ESP_LOGI(TAG, "[Step 8] Initializing Alarm Manager and Event Logger...");
    storage_manager_init();
    alarm_manager_init();
    ESP_LOGI(TAG, "Alarm Manager and Event Logger ready");

    // Step 8: Initialize MQTT Manager (WiFi + MQTT + Telemetry)
    ESP_LOGI(TAG, "[Step 9] Initializing MQTT Manager...");
    mqtt_manager_init();
    ESP_LOGI(TAG, "MQTT Manager started (connecting to WiFi/broker)");

    // Step 9: Initialize Diagnostics Manager (OTA rollback guard + health monitoring)
    ESP_LOGI(TAG, "[Step 10] Initializing Diagnostics Manager...");
    diagnostics_manager_init();
    ESP_LOGI(TAG, "All subsystems initialized - controller operational");

    // Log statistics
    event_bus_stats_t stats;
    if (event_bus_get_stats(&stats))
    {
        ESP_LOGI(TAG, "Event Bus initialized:");
        ESP_LOGI(TAG, "  Events processed: %u", stats.events_processed);
        ESP_LOGI(TAG, "  Queue capacity: %d", EVENT_BUS_QUEUE_LEN);
        ESP_LOGI(TAG, "  Subscribers: %u", stats.subscribers_active);
    }

    ESP_LOGI(TAG, "System ready - implementing remaining components...");

    // Main loop
    while (1)
    {
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}

/**
 * Application entry point
 */
void app_main(void)
{
    // Create system initialization task
    BaseType_t result = xTaskCreate(
        system_init_task,
        "sys_init",
        4096,  // Stack size
        NULL,  // Parameter
        5,     // Priority
        NULL); // Task handle

    if (result != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create initialization task!");
    }
}
