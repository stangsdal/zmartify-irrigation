/**
 * @file diagnostics_manager.h
 * @brief Diagnostics Manager – system health monitoring and OTA rollback guard
 *
 * Collects:
 *   - Uptime, heap usage, reset reason
 *   - Active alarm count and severity
 *   - Event bus statistics
 *   - Per-task high-water marks
 *
 * Provides:
 *   - JSON health report for MQTT publication
 *   - Post-boot OTA validity confirmation (30-second health check)
 *   - diagnostics_manager_is_healthy() for pre-flight checks
 *
 * Architecture ref: MEP v5.0 Volume 2, Chapter 17
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

#include "diagnostics_policy.h"

/* ─── Health snapshot ─────────────────────────────────────────────────── */

typedef struct
{
    uint32_t uptime_s;            /**< Seconds since boot                  */
    uint32_t heap_free_bytes;     /**< Current free heap                   */
    uint32_t heap_min_bytes;      /**< All-time minimum free heap          */
    uint32_t heap_total_bytes;    /**< Total heap size                     */
    uint8_t  heap_utilisation_pct;/**< (total-free)/total × 100           */
    uint8_t  active_alarms;       /**< Total active alarms                 */
    uint8_t  critical_alarms;     /**< Critical-severity active alarms     */
    uint32_t events_processed;    /**< Event bus processed count           */
    uint32_t events_dropped;      /**< Event bus dropped count             */
    uint8_t  reset_reason;        /**< esp_reset_reason_t cast to uint8    */
    uint32_t log_entries;         /**< Current event log entry count       */
    bool     flow_sensor_available;
    bool     pressure_sensor_available;
    bool     mqtt_connected;
    bool     time_synchronized;
    bool     storage_ready;
    bool     storage_last_write_ok;
    uint32_t control_stack_free_bytes;
    uint32_t telemetry_stack_free_bytes;
    uint32_t watersensor_stack_free_bytes;
    diag_policy_result_t status;
} diag_health_t;

typedef bool (*diagnostics_snapshot_fn)(void *context,
                                        diag_policy_input_t *policy_input,
                                        uint8_t *active_alarms,
                                        uint32_t *log_entries);
typedef void (*diagnostics_action_fn)(void *context);
typedef void (*diagnostics_audit_fn)(void *context, const char *message);

typedef struct
{
    diagnostics_snapshot_fn snapshot;
    diagnostics_action_fn raise_critical_alarm;
    diagnostics_action_fn ota_confirmed;
    diagnostics_audit_fn audit;
    void *context;
} diagnostics_manager_config_t;

/* ─── API ─────────────────────────────────────────────────────────────── */

/**
 * @brief Initialise the Diagnostics Manager.
 *
 * Starts a 30-second post-boot health check task for OTA rollback guard.
 * If all subsystems are healthy after 30 s, marks OTA partition as valid.
 *
 * @return true on success
 */
bool diagnostics_manager_init(const diagnostics_manager_config_t *config);

/**
 * @brief Collect a current health snapshot.
 */
bool diagnostics_get_health(diag_health_t *out);

/**
 * @brief Render health snapshot as JSON into caller-supplied buffer.
 *
 * @param buf     Output buffer
 * @param len     Buffer size (minimum 768 bytes recommended)
 * @return Bytes written (excluding null terminator)
 */
size_t diagnostics_health_to_json(char *buf, size_t len);

/**
 * @brief Check whether the system is healthy (no critical alarms, heap OK).
 *
 * Used by the OTA rollback guard and pre-flight checks.
 */
bool diagnostics_is_healthy(void);
