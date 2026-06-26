/*
 * event_bus.c
 * Event Bus Implementation
 *
 * Implements the core event-driven communication mechanism.
 * Uses FreeRTOS queues and semaphores for thread-safe operation.
 */

#include "event_bus.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include <string.h>
#include <stdlib.h>

static const char *TAG = "event_bus";

/**
 * Subscription entry in the subscription database
 */
typedef struct
{
    uint32_t event_id;              ///< Event this subscription watches
    event_callback_t callback;      ///< Callback function
    void *context;                  ///< Subscriber context
    bool active;                    ///< True if subscription is active
} event_subscription_t;

/**
 * Event bus module state
 */
typedef struct
{
    QueueHandle_t event_queue;                  ///< Event dispatch queue
    SemaphoreHandle_t subscription_lock;        ///< Protects subscription DB
    TaskHandle_t dispatcher_task;               ///< Dispatcher task handle
    event_subscription_t subscriptions[EVENT_BUS_MAX_SUBSCRIBERS];
    uint32_t subscription_count;

    // Statistics
    uint32_t events_processed;
    uint32_t events_dropped;
    uint32_t queue_depth_peak;
    uint64_t dispatch_latency_total;
    uint32_t dispatch_count;

    bool initialized;
} event_bus_state_t;

// Module-scope state
static event_bus_state_t s_event_bus = {0};

/**
 * Event dispatcher task
 *
 * Runs continuously, dequeuing events and delivering to subscribers.
 * Priority: medium (configurable via FreeRTOS config)
 */
static void event_dispatcher_task(void *arg)
{
    (void)arg;

    ESP_LOGI(TAG, "Event dispatcher task started");

    while (1)
    {
        zic_event_t event;
        uint64_t start_time = esp_timer_get_time();

        // Wait for event with timeout
        if (xQueueReceive(s_event_bus.event_queue, &event, pdMS_TO_TICKS(1000)) != pdTRUE)
        {
            // Timeout - no events to process
            continue;
        }

        // Check current queue depth for statistics
        UBaseType_t queue_depth = uxQueueMessagesWaiting(s_event_bus.event_queue);
        if (queue_depth > s_event_bus.queue_depth_peak)
        {
            s_event_bus.queue_depth_peak = queue_depth;
        }

        // Acquire lock and deliver to matching subscribers
        if (xSemaphoreTake(s_event_bus.subscription_lock, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            for (uint32_t i = 0; i < s_event_bus.subscription_count; i++)
            {
                event_subscription_t *sub = &s_event_bus.subscriptions[i];

                if (!sub->active)
                {
                    continue;
                }

                // Match event: either specific event_id or 0xFFFFFFFF (all)
                if (sub->event_id == event.event_id || sub->event_id == 0xFFFFFFFF)
                {
                    // Invoke subscriber callback
                    if (sub->callback != NULL)
                    {
                        sub->callback(&event, sub->context);
                    }
                }
            }

            xSemaphoreGive(s_event_bus.subscription_lock);
        }
        else
        {
            ESP_LOGW(TAG, "Failed to acquire subscription lock for event 0x%04x", event.event_id);
        }

        // Update statistics
        s_event_bus.events_processed++;
        uint64_t elapsed_us = esp_timer_get_time() - start_time;
        s_event_bus.dispatch_latency_total += elapsed_us;
        s_event_bus.dispatch_count++;
    }
}

bool event_bus_init(void)
{
    if (s_event_bus.initialized)
    {
        ESP_LOGW(TAG, "Event bus already initialized");
        return true;
    }

    ESP_LOGI(TAG, "Initializing event bus (queue size: %d)", EVENT_BUS_QUEUE_LEN);

    // Create event queue (priority ordered)
    s_event_bus.event_queue = xQueueCreate(EVENT_BUS_QUEUE_LEN, sizeof(zic_event_t));
    if (s_event_bus.event_queue == NULL)
    {
        ESP_LOGE(TAG, "Failed to create event queue");
        return false;
    }

    // Create subscription lock (binary semaphore)
    s_event_bus.subscription_lock = xSemaphoreCreateBinary();
    if (s_event_bus.subscription_lock == NULL)
    {
        ESP_LOGE(TAG, "Failed to create subscription lock");
        vQueueDelete(s_event_bus.event_queue);
        return false;
    }
    xSemaphoreGive(s_event_bus.subscription_lock);

    // Initialize subscription database
    memset(s_event_bus.subscriptions, 0, sizeof(s_event_bus.subscriptions));
    s_event_bus.subscription_count = 0;

    // Reset statistics
    s_event_bus.events_processed = 0;
    s_event_bus.events_dropped = 0;
    s_event_bus.queue_depth_peak = 0;
    s_event_bus.dispatch_latency_total = 0;
    s_event_bus.dispatch_count = 0;

    // Create dispatcher task
    // Note: Priority should be configured via FreeRTOS config
    BaseType_t result = xTaskCreate(
        event_dispatcher_task,
        "event_dispatcher",
        4096,  // Stack size in words
        NULL,  // Parameter
        7,     // Priority (configurable, should be medium)
        &s_event_bus.dispatcher_task);

    if (result != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create dispatcher task");
        vSemaphoreDelete(s_event_bus.subscription_lock);
        vQueueDelete(s_event_bus.event_queue);
        return false;
    }

    s_event_bus.initialized = true;
    ESP_LOGI(TAG, "Event bus initialized successfully");

    return true;
}

void event_bus_shutdown(void)
{
    if (!s_event_bus.initialized)
    {
        return;
    }

    ESP_LOGI(TAG, "Shutting down event bus");

    // Delete dispatcher task
    if (s_event_bus.dispatcher_task != NULL)
    {
        vTaskDelete(s_event_bus.dispatcher_task);
        s_event_bus.dispatcher_task = NULL;
    }

    // Release synchronization primitives
    if (s_event_bus.subscription_lock != NULL)
    {
        vSemaphoreDelete(s_event_bus.subscription_lock);
        s_event_bus.subscription_lock = NULL;
    }

    // Delete event queue
    if (s_event_bus.event_queue != NULL)
    {
        vQueueDelete(s_event_bus.event_queue);
        s_event_bus.event_queue = NULL;
    }

    s_event_bus.initialized = false;
    ESP_LOGI(TAG, "Event bus shutdown complete");
}

bool event_bus_publish(uint32_t event_id, uint32_t producer_id, uint8_t priority,
                       uint8_t payload_type, const void *payload, uint32_t payload_size)
{
    if (!s_event_bus.initialized)
    {
        ESP_LOGE(TAG, "Event bus not initialized");
        return false;
    }

    // Validate inputs
    if (payload_size > EVENT_PAYLOAD_MAX_SIZE)
    {
        ESP_LOGE(TAG, "Payload too large: %u > %u", payload_size, EVENT_PAYLOAD_MAX_SIZE);
        return false;
    }

    // Create event
    zic_event_t event;
    event.event_id = event_id;
    event.producer_id = producer_id;
    event.timestamp_us = esp_timer_get_time();
    event.priority = priority;
    event.payload_type = payload_type;
    event.payload_size = payload_size;

    // Copy payload
    if (payload_size > 0 && payload != NULL)
    {
        memcpy(event.payload, payload, payload_size);
    }

    // Try to send to queue (non-blocking)
    BaseType_t result = xQueueSend(s_event_bus.event_queue, &event, 0);

    if (result != pdTRUE)
    {
        s_event_bus.events_dropped++;
        ESP_LOGW(TAG, "Event queue full, dropped event 0x%04x", event_id);
        return false;
    }

    return true;
}

bool event_bus_subscribe(uint32_t event_id, event_callback_t callback, void *subscriber_context)
{
    if (!s_event_bus.initialized)
    {
        ESP_LOGE(TAG, "Event bus not initialized");
        return false;
    }

    if (callback == NULL)
    {
        ESP_LOGE(TAG, "Invalid callback pointer");
        return false;
    }

    if (xSemaphoreTake(s_event_bus.subscription_lock, pdMS_TO_TICKS(1000)) != pdTRUE)
    {
        ESP_LOGE(TAG, "Failed to acquire subscription lock");
        return false;
    }

    // Check if we've hit max subscribers
    if (s_event_bus.subscription_count >= EVENT_BUS_MAX_SUBSCRIBERS)
    {
        ESP_LOGE(TAG, "Maximum subscribers reached (%d)", EVENT_BUS_MAX_SUBSCRIBERS);
        xSemaphoreGive(s_event_bus.subscription_lock);
        return false;
    }

    // Add subscription
    event_subscription_t *sub = &s_event_bus.subscriptions[s_event_bus.subscription_count];
    sub->event_id = event_id;
    sub->callback = callback;
    sub->context = subscriber_context;
    sub->active = true;

    s_event_bus.subscription_count++;

    xSemaphoreGive(s_event_bus.subscription_lock);

    ESP_LOGD(TAG, "Subscription added for event 0x%04x (total: %u)", event_id, s_event_bus.subscription_count);

    return true;
}

bool event_bus_unsubscribe(uint32_t event_id, event_callback_t callback)
{
    if (!s_event_bus.initialized)
    {
        return false;
    }

    if (xSemaphoreTake(s_event_bus.subscription_lock, pdMS_TO_TICKS(1000)) != pdTRUE)
    {
        ESP_LOGE(TAG, "Failed to acquire subscription lock");
        return false;
    }

    bool found = false;

    for (uint32_t i = 0; i < s_event_bus.subscription_count; i++)
    {
        event_subscription_t *sub = &s_event_bus.subscriptions[i];

        if (sub->event_id == event_id && sub->callback == callback)
        {
            sub->active = false;
            found = true;
            break;
        }
    }

    xSemaphoreGive(s_event_bus.subscription_lock);

    if (found)
    {
        ESP_LOGD(TAG, "Subscription removed for event 0x%04x", event_id);
    }

    return found;
}

bool event_bus_get_stats(event_bus_stats_t *stats)
{
    if (stats == NULL)
    {
        return false;
    }

    if (!s_event_bus.initialized)
    {
        return false;
    }

    stats->events_processed = s_event_bus.events_processed;
    stats->events_dropped = s_event_bus.events_dropped;
    stats->queue_depth_current = uxQueueMessagesWaiting(s_event_bus.event_queue);
    stats->queue_depth_peak = s_event_bus.queue_depth_peak;
    stats->subscribers_active = s_event_bus.subscription_count;

    // Calculate average latency
    if (s_event_bus.dispatch_count > 0)
    {
        stats->dispatch_latency_us = s_event_bus.dispatch_latency_total / s_event_bus.dispatch_count;
    }
    else
    {
        stats->dispatch_latency_us = 0;
    }

    return true;
}

void event_bus_stats_reset(void)
{
    if (!s_event_bus.initialized)
    {
        return;
    }

    s_event_bus.events_processed = 0;
    s_event_bus.events_dropped = 0;
    s_event_bus.queue_depth_peak = 0;
    s_event_bus.dispatch_latency_total = 0;
    s_event_bus.dispatch_count = 0;

    ESP_LOGI(TAG, "Event bus statistics reset");
}
