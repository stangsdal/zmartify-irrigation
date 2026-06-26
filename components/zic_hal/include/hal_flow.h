/**
 * @file hal_flow.h
 * @brief Flow HAL - Pulse counter interface for Hall-effect flow meters
 *
 * Uses the ESP32-S3 PCNT (Pulse Counter) peripheral.
 * Counts rising edges from Hall-effect flow sensors.
 * The Flow Manager converts pulse counts to L/min; the HAL does not.
 *
 * Architecture ref: MEP v5.0 Volume 2, Chapter 4, Section 4.8
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum hal_result hal_result_t;

/** Number of supported flow meter channels */
#define HAL_FLOW_CHANNEL_COUNT  2

/**
 * @brief Flow meter channel identifier
 */
typedef enum
{
    HAL_FLOW_CHANNEL_0 = 0,  /**< Primary flow meter */
    HAL_FLOW_CHANNEL_1 = 1,  /**< Secondary flow meter */
} hal_flow_channel_t;

/**
 * @brief Initialise all flow counter channels.
 *
 * Configures PCNT units for rising-edge counting on defined GPIO pins.
 * Counters start at zero after init.
 *
 * @return HAL_OK on success.
 */
hal_result_t hal_flow_init(void);

/**
 * @brief Enable pulse counting on a channel.
 */
hal_result_t hal_flow_start(hal_flow_channel_t channel);

/**
 * @brief Disable pulse counting on a channel (counter value preserved).
 */
hal_result_t hal_flow_stop(hal_flow_channel_t channel);

/**
 * @brief Read the current pulse count for a channel.
 *
 * @param channel  Channel to read
 * @param count    Output: accumulated pulse count since last reset
 * @return HAL_OK on success.
 */
hal_result_t hal_flow_read(hal_flow_channel_t channel, uint32_t *count);

/**
 * @brief Reset the pulse counter for a channel to zero.
 */
hal_result_t hal_flow_reset(hal_flow_channel_t channel);

/**
 * @brief Read and atomically reset a channel counter.
 *
 * Useful for interval-based frequency calculations.
 * @param channel  Channel
 * @param count    Output: pulse count since last read_reset
 */
hal_result_t hal_flow_read_reset(hal_flow_channel_t channel, uint32_t *count);
