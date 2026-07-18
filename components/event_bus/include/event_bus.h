/*
 * event_bus.h
 * Central event-driven communication for Zmartify Irrigation Controller
 *
 * Provides asynchronous event dispatch and subscription management following
 * the event-driven architecture pattern (EDS-003). All firmware subsystems
 * communicate through this event bus rather than direct module-to-module calls.
 *
 * Copyright (c) 2026 Zmartify
 * Licensed under ESP-IDF License
 */

#ifndef EVENT_BUS_H
#define EVENT_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/**
 * @defgroup event_bus Event Bus
 * @brief Central communication mechanism for event-driven architecture
 * @{
 */

/**
 * Maximum number of events in the dispatch queue
 */
#define EVENT_BUS_QUEUE_LEN 64

/**
 * Maximum payload size per event (bytes)
 */
#define EVENT_PAYLOAD_MAX_SIZE 256

/**
 * Maximum subscribers per event type
 */
#define EVENT_BUS_MAX_SUBSCRIBERS 16

/**
 * Event priority levels (0 = lowest, 15 = highest/critical)
 *
 * @note Safety-critical events (14-15) always processed before others
 */
typedef enum
{
    EVENT_PRIORITY_LOWEST = 0,
    EVENT_PRIORITY_LOW = 3,
    EVENT_PRIORITY_NORMAL = 7,
    EVENT_PRIORITY_HIGH = 10,
    EVENT_PRIORITY_CRITICAL = 14,
    EVENT_PRIORITY_SAFETY = 15,
} event_priority_e;

/**
 * Event identifiers (assigned by producer modules)
 *
 * @note Event IDs must be unique within the system
 */
typedef enum
{
    // Zone events
    EVENT_ZONE_STARTED = 0x0100,
    EVENT_ZONE_STOPPED = 0x0101,
    EVENT_ZONE_FAULT = 0x0102,
    EVENT_ZONE_STATE_CHANGED = 0x0103,

    // Relay events
    EVENT_RELAY_ACTIVATED = 0x0200,
    EVENT_RELAY_DEACTIVATED = 0x0201,
    EVENT_RELAY_STUCK_DETECTED = 0x0202,

    // Flow/Pressure events
    EVENT_FLOW_UPDATED = 0x0300,
    EVENT_FLOW_ANOMALY = 0x0301,
    EVENT_PRESSURE_UPDATED = 0x0302,
    EVENT_PRESSURE_OUT_OF_BOUNDS = 0x0303,

    // External Water Sensor events
    EVENT_WATERSENSOR_ONLINE = 0x0310,
    EVENT_WATERSENSOR_OFFLINE = 0x0311,
    EVENT_WATERSENSOR_PROTOCOL_ERROR = 0x0312,
    EVENT_WATERSENSOR_SNAPSHOT = 0x0313,
    EVENT_SENSOR_STALE = 0x0314,
    EVENT_SENSOR_FAULT = 0x0315,

    // Irrigation events
    EVENT_IRRIGATION_STARTED = 0x0400,
    EVENT_IRRIGATION_COMPLETED = 0x0401,
    EVENT_IRRIGATION_FAULT = 0x0402,

    // Weather events
    EVENT_WEATHER_UPDATED = 0x0500,
    EVENT_ET_CALCULATED = 0x0501,
    EVENT_RAIN_DETECTED = 0x0502,

    // Alarm events
    EVENT_ALARM_GENERATED = 0x0600,
    EVENT_ALARM_ACKNOWLEDGED = 0x0601,
    EVENT_ALARM_RESOLVED = 0x0602,

    // MQTT events
    EVENT_MQTT_CONNECTED = 0x0700,
    EVENT_MQTT_DISCONNECTED = 0x0701,
    EVENT_MQTT_COMMAND = 0x0702,

    // OTA events
    EVENT_OTA_STARTED = 0x0800,
    EVENT_OTA_COMPLETE = 0x0801,
    EVENT_OTA_FAILED = 0x0802,

    // System events
    EVENT_SYSTEM_BOOT = 0x0900,
    EVENT_SYSTEM_FAULT = 0x0901,
    EVENT_WATCHDOG_TRIGGERED = 0x0902,

    // Diagnostic events
    EVENT_DIAGNOSTICS_REPORT = 0x0A00,
} event_id_e;

/**
 * Event structure containing metadata and payload
 *
 * @note Events are immutable after creation and delivery
 */
typedef struct
{
    uint32_t event_id;          ///< Unique event identifier (event_id_e)
    uint32_t producer_id;       ///< Module that generated this event
    uint64_t timestamp_us;      ///< System uptime (microseconds)
    uint8_t priority;           ///< Priority level (0-15, higher = earlier)
    uint8_t payload_type;       ///< Payload format identifier
    uint32_t payload_size;      ///< Size of payload data (bytes)
    uint8_t payload[EVENT_PAYLOAD_MAX_SIZE];  ///< Event-specific data
} zic_event_t;

/**
 * Event subscription callback function signature
 *
 * @param event Pointer to const event (do not modify)
 * @param subscriber_context Opaque subscriber context (passed at subscribe time)
 */
typedef void (*event_callback_t)(const zic_event_t *event, void *subscriber_context);

/**
 * Initialize the event bus
 *
 * Sets up the event queue, subscription database, and dispatcher task.
 * Must be called once during system startup before any events are generated.
 *
 * @return true if initialization successful
 * @return false if initialization failed (insufficient memory, etc.)
 */
bool event_bus_init(void);

/**
 * Shutdown the event bus
 *
 * Stops the dispatcher task and releases resources.
 * Should be called during system shutdown.
 */
void event_bus_shutdown(void);

/**
 * Publish an event to the event bus
 *
 * Creates an event with automatic timestamp and delivers it to all subscribers.
 * Non-blocking; event is queued for immediate dispatch.
 *
 * @param event_id Unique event identifier
 * @param producer_id Module generating this event
 * @param priority Event priority level (EVENT_PRIORITY_* constants)
 * @param payload_type Format identifier for payload interpretation
 * @param payload Pointer to payload data (may be NULL if no payload)
 * @param payload_size Size of payload (0 if no payload)
 *
 * @return true if event was queued successfully
 * @return false if queue is full or event_id invalid
 *
 * @note Thread-safe; may be called from any context including ISRs
 * @note Payload is copied into event (no reference retained)
 */
bool event_bus_publish(uint32_t event_id, uint32_t producer_id, uint8_t priority,
                       uint8_t payload_type, const void *payload, uint32_t payload_size);

/**
 * Subscribe to events
 *
 * Registers a callback to receive events matching the specified filter.
 * The callback is invoked by the event dispatcher task for matching events.
 *
 * @param event_id Event identifier to subscribe to (0xFFFFFFFF = subscribe to all)
 * @param callback Function to invoke when matching event is received
 * @param subscriber_context Opaque context passed to callback
 *
 * @return true if subscription successful
 * @return false if maximum subscribers reached or invalid callback
 *
 * @note Thread-safe; may be called from multiple contexts
 * @note Callback should be fast (< 1ms) to avoid blocking dispatcher
 */
bool event_bus_subscribe(uint32_t event_id, event_callback_t callback, void *subscriber_context);

/**
 * Unsubscribe from events
 *
 * Removes a previously registered subscription.
 *
 * @param event_id Event identifier to unsubscribe from
 * @param callback Callback function to remove
 *
 * @return true if unsubscription successful
 * @return false if subscription not found
 */
bool event_bus_unsubscribe(uint32_t event_id, event_callback_t callback);

/**
 * Get event bus statistics
 *
 * Provides diagnostics information about bus operation.
 */
typedef struct
{
    uint32_t events_processed;      ///< Total events dispatched
    uint32_t events_dropped;        ///< Events discarded (queue full)
    uint32_t queue_depth_current;   ///< Events currently in queue
    uint32_t queue_depth_peak;      ///< Maximum queue depth observed
    uint32_t subscribers_active;    ///< Currently active subscriptions
    uint64_t dispatch_latency_us;   ///< Average event dispatch latency (microseconds)
} event_bus_stats_t;

/**
 * Query event bus statistics
 *
 * @param stats Pointer to statistics structure to populate
 *
 * @return true if stats retrieved successfully
 * @return false if stats pointer invalid
 */
bool event_bus_get_stats(event_bus_stats_t *stats);

/**
 * Reset event bus statistics
 *
 * Clears counters and peak values.
 */
void event_bus_stats_reset(void);

/** @} */

#ifdef __cplusplus
}
#endif

#endif /* EVENT_BUS_H */
