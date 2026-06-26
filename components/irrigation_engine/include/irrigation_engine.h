/**
 * @file irrigation_engine.h
 * @brief Irrigation Engine – central irrigation decision and execution engine
 *
 * The Irrigation Engine is the only component authorised to start or stop
 * irrigation. All requests (automatic or manual) pass through this API.
 *
 * Engine State Machine:
 *   DISABLED
 *   IDLE          ← normal resting state
 *   PREPARING     ← validating pre-conditions
 *   MASTER_OPEN   ← master valve opened, pressure stabilising
 *   ZONE_RUNNING  ← zone valve open, runtime counting
 *   STOPPING      ← closing valves in sequence
 *   RAIN_DELAY    ← weather skip active
 *   FAULT         ← recoverable fault, waiting for operator
 *   EMERGENCY     ← unrecoverable, all valves closed
 *
 * Architecture ref: MEP v5.0 Volume 2, Chapter 5
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/* ─── Engine states ───────────────────────────────────────────────────── */

typedef enum
{
    ENGINE_STATE_DISABLED     = 0,
    ENGINE_STATE_IDLE         = 1,
    ENGINE_STATE_PREPARING    = 2,
    ENGINE_STATE_MASTER_OPEN  = 3,
    ENGINE_STATE_ZONE_RUNNING = 4,
    ENGINE_STATE_STOPPING     = 5,
    ENGINE_STATE_RAIN_DELAY   = 6,
    ENGINE_STATE_FAULT        = 7,
    ENGINE_STATE_EMERGENCY    = 8,
} engine_state_t;

/* ─── Request structure ───────────────────────────────────────────────── */

/**
 * @brief Manual irrigation request (zone + duration)
 */
typedef struct
{
    uint8_t  zone_id;         /**< 1-based zone (1–15); 0 = run all zones */
    uint32_t runtime_s;       /**< Duration in seconds; 0 = use config default */
} irrigation_request_t;

/* ─── Lifecycle ───────────────────────────────────────────────────────── */

/**
 * @brief Initialise the Irrigation Engine and start its supervision task.
 *
 * Depends on: event_bus, relay_manager, zone_manager, config_manager.
 * @return true on success
 */
bool irrigation_engine_init(void);

/* ─── Control commands ────────────────────────────────────────────────── */

/**
 * @brief Request to start a zone (manual override).
 *
 * Thread-safe – may be called from any task.
 * The engine queues the request and validates before acting.
 *
 * @param req  Zone and runtime specification
 * @return true if request was accepted into the queue
 */
bool irrigation_start_zone(const irrigation_request_t *req);

/**
 * @brief Request to stop a specific zone.
 */
bool irrigation_stop_zone(uint8_t zone_id);

/**
 * @brief Request to stop all zones and close master valve immediately.
 */
bool irrigation_stop_all(void);

/**
 * @brief Trigger an emergency stop (highest priority shutdown).
 */
void irrigation_emergency_stop(const char *reason);

/* ─── Status ──────────────────────────────────────────────────────────── */

/**
 * @brief Get current engine state.
 */
engine_state_t irrigation_get_state(void);

/**
 * @brief Get the zone currently being irrigated (0 if none).
 */
uint8_t irrigation_active_zone(void);

/**
 * @brief Check whether the engine is ready to accept new requests.
 */
bool irrigation_is_idle(void);
