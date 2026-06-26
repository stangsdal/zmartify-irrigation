/**
 * @file relay_manager.h
 * @brief Relay Manager – safe, event-driven relay control
 *
 * The Relay Manager is the only firmware component permitted to
 * energise or de-energise irrigation relays. It sits between the
 * Irrigation Engine and the HAL relay driver, providing:
 *
 *  - Logical ON/OFF semantics (compensates HL-58S active-low internally)
 *  - Master valve interlock (zone relay blocked until master valve open)
 *  - Maximum simultaneous valve enforcement
 *  - Shadow state tracking (query without I²C round-trip)
 *  - Event Bus publication on every state change
 *
 * Architecture ref: MEP v5.0 Volume 2, Chapter 7
 *
 * Relay mapping (HW-IO-002):
 *   Relay 0  = Master Valve
 *   Relay 1–15 = Zones 1–15
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/* ─── Constants ───────────────────────────────────────────────────────── */

#define RELAY_MASTER_VALVE      0u   /**< Relay index for master valve */
#define RELAY_ZONE_FIRST        1u   /**< First zone relay index */
#define RELAY_ZONE_LAST        15u   /**< Last zone relay index */
#define RELAY_COUNT            16u

/* ─── Types ───────────────────────────────────────────────────────────── */

/**
 * @brief Relay Manager result codes
 */
typedef enum
{
    RELAY_OK                = 0,
    RELAY_NOT_INITIALIZED   = 1,
    RELAY_INVALID_INDEX     = 2,
    RELAY_MASTER_NOT_OPEN   = 3,   /**< Zone opened without master valve */
    RELAY_MAX_CONCURRENT    = 4,   /**< Too many valves open simultaneously */
    RELAY_HAL_ERROR         = 5,
} relay_result_t;

/* ─── Lifecycle ───────────────────────────────────────────────────────── */

/**
 * @brief Initialise the Relay Manager and guarantee all relays are closed.
 *
 * @param max_simultaneous  Maximum number of zone valves that may be
 *                          open at the same time (1 is the safe default).
 */
relay_result_t relay_manager_init(uint8_t max_simultaneous);

/* ─── Valve control ───────────────────────────────────────────────────── */

/**
 * @brief Open the master valve (relay 0).
 */
relay_result_t relay_master_open(void);

/**
 * @brief Close the master valve.
 */
relay_result_t relay_master_close(void);

/**
 * @brief Open a zone valve (relay 1–15).
 *
 * Requires master valve to be open first.
 * Enforces max_simultaneous limit.
 *
 * @param relay  Relay index 1–15 (corresponds to Zone 1–15)
 */
relay_result_t relay_zone_open(uint8_t relay);

/**
 * @brief Close a zone valve.
 */
relay_result_t relay_zone_close(uint8_t relay);

/**
 * @brief Emergency close: shut all valves (master + zones) immediately.
 *
 * Always succeeds or logs the error; never returns failure silently.
 */
relay_result_t relay_close_all(void);

/* ─── State queries ───────────────────────────────────────────────────── */

/**
 * @brief Query whether the master valve is currently open.
 */
bool relay_master_is_open(void);

/**
 * @brief Query whether a zone relay is open.
 *
 * @param relay  1–15
 */
bool relay_zone_is_open(uint8_t relay);

/**
 * @brief Count of zone valves currently open.
 */
uint8_t relay_open_zone_count(void);
