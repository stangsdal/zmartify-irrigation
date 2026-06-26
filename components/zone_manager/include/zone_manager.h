/**
 * @file zone_manager.h
 * @brief Zone Manager – zone runtime state, statistics, and valve sequencing
 *
 * The Zone Manager maintains per-zone runtime state and statistics.
 * It is the sole authority for initiating and stopping zone valve operations,
 * always working through the Relay Manager.
 *
 * Architecture ref: MEP v5.0 Volume 2, Chapter 6
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/* ─── Zone state machine ─────────────────────────────────────────────── */

/**
 * @brief Per-zone runtime state
 */
typedef enum
{
    ZONE_STATE_IDLE      = 0,  /**< Valve closed, not running */
    ZONE_STATE_OPENING   = 1,  /**< Relay energised, waiting for flow */
    ZONE_STATE_RUNNING   = 2,  /**< Valve open, runtime counting */
    ZONE_STATE_CLOSING   = 3,  /**< Relay de-energised, draining */
    ZONE_STATE_FAULT     = 4,  /**< Hardware or flow fault */
} zone_state_t;

/* ─── Zone runtime data ──────────────────────────────────────────────── */

/**
 * @brief Runtime status for a single zone (1-based zone_id)
 */
typedef struct
{
    uint8_t      zone_id;             /**< 1-based zone number */
    zone_state_t state;
    uint32_t     requested_runtime_s; /**< Runtime asked for */
    uint32_t     elapsed_s;           /**< Time in RUNNING state */
    uint32_t     start_epoch;         /**< UNIX epoch at valve-open */
    uint32_t     last_run_epoch;      /**< Epoch of most recent completed run */
    uint32_t     total_runtime_s;     /**< Lifetime accumulated runtime */
    uint32_t     run_count;           /**< Number of completed runs */
} zone_runtime_t;

/* ─── API ─────────────────────────────────────────────────────────────── */

/**
 * @brief Initialise the Zone Manager.
 *
 * Resets all zone runtime records to IDLE.
 * relay_manager and config_manager must be initialised first.
 */
void zone_manager_init(void);

/**
 * @brief Open a zone valve and start the runtime timer.
 *
 * Requires master valve to already be open.
 *
 * @param zone_id          1-based zone number (1–15)
 * @param runtime_seconds  How long to run; 0 = use zone config default
 * @return true on success
 */
bool zone_start(uint8_t zone_id, uint32_t runtime_seconds);

/**
 * @brief Close a zone valve and record statistics.
 *
 * @param zone_id  1-based zone number
 * @return true on success
 */
bool zone_stop(uint8_t zone_id);

/**
 * @brief Close all open zone valves (does not close master valve).
 */
void zone_stop_all(void);

/**
 * @brief Tick function – must be called every second by the engine task.
 *
 * Advances elapsed timers and triggers stop when runtime is reached.
 */
void zone_manager_tick(void);

/**
 * @brief Get a snapshot of a zone's current runtime state.
 *
 * @param zone_id   1-based
 * @param out       Destination for the snapshot
 * @return true on success
 */
bool zone_get_runtime(uint8_t zone_id, zone_runtime_t *out);

/**
 * @brief Query whether any zone is currently running.
 */
bool zone_any_active(void);

/**
 * @brief Count of zones currently in OPENING or RUNNING state.
 */
uint8_t zone_active_count(void);
