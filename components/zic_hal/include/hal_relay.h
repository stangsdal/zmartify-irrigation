/**
 * @file hal_relay.h
 * @brief Relay HAL - Zone solenoid & master valve control via MCP23017
 *
 * All relay control goes through this layer.
 * The caller sees only relay index numbers (0-15).
 * MCP23017 register addresses and I²C details are hidden inside hal_relay.c.
 *
 * Architecture ref: MEP v5.0 Volume 2, Chapter 4, Section 4.7
 *
 * Relay mapping:
 *   Relay  0-7  : MCP23017 GPIOA 0-7
 *   Relay  8-15 : MCP23017 GPIOB 0-7
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum hal_result hal_result_t;

/** Maximum number of controllable relays */
#define HAL_RELAY_COUNT    16

/**
 * @brief Initialise the relay subsystem.
 *
 * Configures MCP23017 devices and turns all relays OFF.
 * @return HAL_OK on success; HAL_DEVICE_NOT_FOUND if MCP23017 not present.
 */
hal_result_t hal_relay_init(void);

/**
 * @brief Energise relay (open valve / turn on).
 *
 * @param relay  0-based relay index (0–15)
 * @return HAL_OK on success; HAL_INVALID_PARAMETER if index out of range.
 */
hal_result_t hal_relay_on(uint8_t relay);

/**
 * @brief De-energise relay (close valve / turn off).
 *
 * @param relay  0-based relay index (0–15)
 * @return HAL_OK on success.
 */
hal_result_t hal_relay_off(uint8_t relay);

/**
 * @brief Query relay state without changing it.
 *
 * @param relay  0-based relay index
 * @return true if relay is currently energised.
 */
bool hal_relay_get(uint8_t relay);

/**
 * @brief De-energise all relays immediately.
 *
 * Safety function – must succeed regardless of individual relay state.
 * @return HAL_OK on success.
 */
hal_result_t hal_relay_all_off(void);
