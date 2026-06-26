/**
 * @file pressure_manager.h
 * @brief Pressure Manager – hydraulic pressure supervision (Zmartify ZHSS)
 *
 * Samples the ADS1115 ADC (0–10 bar industrial transmitter, 0.5–4.5 V output)
 * at 250 ms intervals, applies a 5-sample moving average, and supervises for:
 *   - Low pressure (supply failure, burst, empty supply)
 *   - High pressure (overpressure, blockage)
 *   - Pressure collapse during active irrigation
 *
 * Publishes EVENT_PRESSURE_UPDATED every sample.
 * Publishes EVENT_PRESSURE_OUT_OF_BOUNDS on fault.
 *
 * Architecture ref: MEP v5.0 Volume 2, Chapter 11
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/* ─── Pressure states ─────────────────────────────────────────────────── */

typedef enum
{
    PRESSURE_STATE_OK        = 0,
    PRESSURE_STATE_LOW       = 1,  /**< Below configured minimum            */
    PRESSURE_STATE_HIGH      = 2,  /**< Above configured maximum            */
    PRESSURE_STATE_COLLAPSE  = 3,  /**< Rapid drop during active irrigation */
    PRESSURE_STATE_FAULT     = 4,  /**< ADC read error                      */
} pressure_state_t;

/* ─── Pressure reading snapshot ────────────────────────────────────────── */

typedef struct
{
    float            pressure_bar;   /**< Current calibrated pressure in bar */
    float            avg_bar;        /**< 5-sample moving average            */
    int32_t          raw_mv;         /**< Raw ADC voltage in millivolts      */
    pressure_state_t state;
    uint32_t         timestamp_ms;
} pressure_reading_t;

/* ─── API ─────────────────────────────────────────────────────────────── */

/**
 * @brief Initialise and start the Pressure Manager task (priority 9, 3 KB).
 */
bool pressure_manager_init(void);

/**
 * @brief Get the latest pressure reading (thread-safe snapshot).
 */
bool pressure_manager_get_reading(pressure_reading_t *out);

/**
 * @brief Get current pressure in bar.
 */
float pressure_manager_get_bar(void);

/**
 * @brief Get current pressure state.
 */
pressure_state_t pressure_manager_get_state(void);
