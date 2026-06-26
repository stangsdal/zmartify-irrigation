/**
 * @file weather_manager.h
 * @brief Weather Manager – environmental data processing and irrigation decisions
 *
 * Receives weather measurements (from any source: local sensor, HTTP API, manual),
 * applies configurable thresholds, manages rain delay, and publishes irrigation
 * recommendations via the event bus.
 *
 * Architecture ref: MEP v5.0 Volume 2, Chapter 8
 */
#pragma once

#include <stdbool.h>
#include <stdint.h>

/* ─── Weather snapshot ───────────────────────────────────────────────── */

/**
 * @brief Current environmental conditions snapshot
 *
 * All fields default to -1.0 (unknown) when not yet measured.
 */
typedef struct
{
    float temperature_c;          /**< Air temperature (°C)              */
    float humidity_pct;           /**< Relative humidity (0-100 %)       */
    float wind_speed_mps;         /**< Wind speed (m/s)                  */
    float solar_radiation_mj_m2;  /**< Solar radiation (MJ/m²/day)       */
    float uv_index;               /**< UV index (0-11+)                  */
    float rain_mm_last_24h;       /**< Observed rainfall last 24 h (mm)  */
    float rain_probability_pct;   /**< Forecast rain probability (0-100) */
    float forecast_rain_mm;       /**< Forecast total rain next 24 h (mm)*/
    uint32_t updated_epoch;       /**< Epoch of last successful update   */
    bool     data_valid;          /**< At least one reading available    */
} weather_snapshot_t;

/* ─── Irrigation recommendation ─────────────────────────────────────── */

/**
 * @brief Irrigation decision flags produced from weather evaluation
 */
typedef struct
{
    bool skip_watering;      /**< Skip entirely (recent/forecast rain)   */
    bool reduce_watering;    /**< Apply reduction factor                 */
    bool increase_watering;  /**< Apply increase factor (hot/sunny)      */
    bool suspend_watering;   /**< Suspend due to wind or freeze          */
    bool block_watering;     /**< Hard block (freeze protection < 2 °C)  */
    float adjustment_factor; /**< Runtime multiplier (0.2 – 2.0)         */
} weather_decision_t;

/* ─── Rain delay ─────────────────────────────────────────────────────── */

/**
 * @brief Activate manual rain delay.
 * @param hours  0 = clear delay; 1-168 = hours to delay
 */
void weather_set_rain_delay(uint16_t hours);

/**
 * @brief Check whether rain delay is currently active.
 */
bool weather_rain_delay_active(void);

/**
 * @brief Remaining rain delay in seconds (0 if not active).
 */
uint32_t weather_rain_delay_remaining_s(void);

/* ─── Data ingestion ──────────────────────────────────────────────────── */

/**
 * @brief Update the weather snapshot with new measurements.
 *
 * Thread-safe. Call from HTTP callback, local sensor task, or test code.
 * Triggers re-evaluation and event publication.
 */
void weather_update(const weather_snapshot_t *snapshot);

/* ─── Query ───────────────────────────────────────────────────────────── */

/**
 * @brief Get current weather snapshot (read-only copy).
 */
void weather_get_snapshot(weather_snapshot_t *out);

/**
 * @brief Evaluate current snapshot against thresholds and return decision.
 */
void weather_get_decision(weather_decision_t *out);

/**
 * @brief Check whether it is safe to irrigate right now.
 *
 * Returns false if: rain delay active, freeze, high wind, skip condition.
 */
bool weather_irrigation_allowed(void);

/* ─── Lifecycle ───────────────────────────────────────────────────────── */

/**
 * @brief Initialise the Weather Manager.
 *
 * Loads rain delay state from NVS. No background task is created here;
 * weather data is pushed in via weather_update().
 */
bool weather_manager_init(void);
