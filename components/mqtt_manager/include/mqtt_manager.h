/**
 * @file mqtt_manager.h
 * @brief MQTT Manager - WiFi + MQTT singleton with full telemetry pipeline
 *
 * Responsibilities:
 *   - WiFi connection management with auto-reconnect
 *   - MQTT broker connection with LWT
 *   - Subscribes to event bus, publishes key events to MQTT broker
 *   - Receives MQTT commands and dispatches to irrigation engine
 *   - Offline-safe: MQTT loss never stops irrigation
 *
 * Topics (base: zmartify/irrigation/{controller_id}):
 *   .../state     - retained JSON controller state (published on change)
 *   .../telemetry - periodic flow/pressure/weather JSON (~30s)
 *   .../alarm     - alarm notification JSON (on alarm raised)
 *   .../command/start_zone  - {"zone_id":1,"runtime_s":600}
 *   .../command/stop_zone   - {"zone_id":1}
 *   .../command/stop_all    - {}
 *   .../command/rain_delay  - {"hours":24}
 *   .../command/ota         - {"url":"http://..."}
 *
 * Architecture ref: MEP v5.0 Volume 2, Chapter 13
 */
#pragma once
#include <stdbool.h>
#include <stdint.h>

/* ─── Connection state ───────────────────────────────────────────────── */

typedef enum {
    MQTT_MGR_WIFI_DISCONNECTED  = 0,
    MQTT_MGR_WIFI_CONNECTING    = 1,
    MQTT_MGR_WIFI_CONNECTED     = 2,
    MQTT_MGR_MQTT_CONNECTING    = 3,
    MQTT_MGR_MQTT_CONNECTED     = 4,
} mqtt_mgr_state_t;

/* ─── Lifecycle ───────────────────────────────────────────────────────── */

/**
 * @brief Initialise WiFi and start the MQTT manager task.
 *
 * Reads SSID, password, broker URI and credentials from config_manager.
 * The manager task runs at priority 5 (below irrigation, above telemetry).
 *
 * @return true if task created successfully
 */
bool mqtt_manager_init(void);

/* ─── Status queries ──────────────────────────────────────────────────── */

bool          mqtt_manager_is_connected(void);
mqtt_mgr_state_t mqtt_manager_get_state(void);

/* ─── Manual publish (for use by other components) ──────────────────── */

/**
 * @brief Publish a raw payload to a topic.
 *
 * Non-blocking; returns false if not connected or queue full.
 *
 * @param topic    Full MQTT topic string
 * @param payload  Null-terminated JSON or text payload
 * @param qos      0, 1, or 2
 * @param retain   Retained message flag
 */
bool mqtt_manager_publish(const char *topic, const char *payload,
                           int qos, bool retain);
