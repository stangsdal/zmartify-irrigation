/**
 * @file flow_manager.h
 * @brief Flow Manager – hydraulic flow measurement and leak detection
 *
 * Continuously samples the DN50 Hall-effect flow meter via hal_flow,
 * calculates L/min, applies a moving average, and supervises for:
 *   - Unexpected flow while idle (leak)
 *   - Missing flow while a zone is running (dry-run / blocked)
 *   - High flow (pipe burst / leak)
 *
 * Publishes EVENT_FLOW_UPDATED every sample period.
 * Publishes EVENT_FLOW_ANOMALY on fault conditions.
 *
 * Architecture ref: MEP v5.0 Volume 2, Chapter 10
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/* ─── Flow states ─────────────────────────────────────────────────────── */

typedef enum
{
    FLOW_STATE_OK         = 0,  /**< Flow within expected range          */
    FLOW_STATE_NO_FLOW    = 1,  /**< Zone open but no flow detected      */
    FLOW_STATE_HIGH       = 2,  /**< Flow above configured maximum       */
    FLOW_STATE_UNEXPECTED = 3,  /**< Flow detected while system idle     */
    FLOW_STATE_FAULT      = 4,  /**< Sensor read error                   */
} flow_state_t;

/* ─── Flow reading snapshot ────────────────────────────────────────────── */

typedef struct
{
    float        rate_lpm;          /**< Current flow rate in L/min        */
    float        avg_lpm;           /**< 5-sample moving average           */
    uint32_t     pulse_count;       /**< Raw pulse accumulator             */
    flow_state_t state;
    uint32_t     timestamp_ms;      /**< System uptime at last sample      */
    uint32_t     total_litres_x10;  /**< Lifetime × 10 (0.1 L resolution) */
} flow_reading_t;

/* ─── API ─────────────────────────────────────────────────────────────── */

/**
 * @brief Initialise and start the Flow Manager task (priority 9, 3 KB stack).
 */
bool flow_manager_init(void);

/**
 * @brief Notify the Flow Manager that a zone has opened.
 *
 * Sets the active-zone flag so that no-flow detection is enabled.
 * Call after the zone valve has been opened.
 *
 * @param zone_id  1-based zone number; 0 = all closed (idle supervision)
 */
void flow_manager_set_active_zone(uint8_t zone_id);

/**
 * @brief Get the latest flow reading (thread-safe snapshot).
 */
bool flow_manager_get_reading(flow_reading_t *out);

/**
 * @brief Get current flow rate in L/min.
 */
float flow_manager_get_rate_lpm(void);

/**
 * @brief Get current flow state.
 */
flow_state_t flow_manager_get_state(void);

/**
 * @brief Reset the lifetime water counter.
 */
void flow_manager_reset_lifetime(void);
