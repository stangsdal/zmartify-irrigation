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

static const char *TAG = "zic_main";

/**
 * System initialization task
 */
static void system_init_task(void *arg)
{
    (void)arg;

    ESP_LOGI(TAG, "===================================");
    ESP_LOGI(TAG, "Zmartify Irrigation Controller v5.0");
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
