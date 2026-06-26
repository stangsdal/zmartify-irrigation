/**
 * @file et_engine.h
 * @brief ET Engine – evapotranspiration calculation and runtime adjustment
 *
 * Implements the Hargreaves simplified ET model (FAO-56 compatible) to
 * calculate daily crop water requirement.
 *
 * ET-adjusted runtime = base_runtime × (ET_today / ET_reference) × Kc × seasonal_factor
 *
 * Architecture ref: MEP v5.0 Volume 2, Chapter 9
 */
#pragma once

#include <stdint.h>
#include <stdbool.h>

/* ─── Environmental inputs ───────────────────────────────────────────── */

typedef struct
{
    float temperature_c;          /**< Mean air temperature (°C)        */
    float humidity_pct;           /**< Relative humidity (%)            */
    float wind_speed_mps;         /**< Wind speed (m/s)                 */
    float solar_radiation_mj_m2;  /**< Solar radiation (MJ/m²/day)      */
    float rain_mm;                /**< Observed rainfall (mm)           */
} et_input_t;

/* ─── ET output ──────────────────────────────────────────────────────── */

typedef struct
{
    float   daily_et_mm;         /**< Reference ET (ETo) for today (mm) */
    float   weekly_et_mm;        /**< 7-day rolling ET accumulation (mm)*/
    float   effective_rain_mm;   /**< Effective rainfall (mm, 0.75 × observed) */
    float   net_requirement_mm;  /**< ET - effective rain (irrigation need) */
    uint8_t confidence;          /**< 0-100 quality score               */
} et_output_t;

/* ─── API ─────────────────────────────────────────────────────────────── */

/**
 * @brief Initialise the ET Engine (loads 7-day history from NVS).
 */
bool et_engine_init(void);

/**
 * @brief Compute daily ET from environmental inputs.
 *
 * Uses Hargreaves simplified model: ETo ≈ 0.0023 × (T + 17.8) × Rs
 * which is calibrated for irrigated turfgrass in temperate climates.
 *
 * @param input   Environmental measurements
 * @param output  Calculated ET values; must not be NULL
 */
void et_engine_compute(const et_input_t *input, et_output_t *output);

/**
 * @brief Calculate ET-adjusted runtime for a zone.
 *
 * Applies: runtime = base × (net_requirement / reference_mm_day) × Kc × seasonal
 *
 * @param base_runtime_s   Configured base runtime (seconds)
 * @param et_mm_day        Today's net ET requirement (mm)
 * @param reference_mm_day Reference ET for sizing (default: 5.0 mm/day)
 * @param kc               Crop coefficient (e.g. 0.8 for turf)
 * @param seasonal_pct     Seasonal factor (0-200, 100 = nominal)
 * @return Adjusted runtime in seconds (capped 20%–200% of base)
 */
uint32_t et_engine_adjust_runtime(uint32_t base_runtime_s,
                                   float    et_mm_day,
                                   float    reference_mm_day,
                                   float    kc,
                                   uint8_t  seasonal_pct);

/**
 * @brief Get the last computed ET output.
 */
bool et_engine_get_last(et_output_t *out);
