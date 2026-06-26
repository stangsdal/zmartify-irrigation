/**
 * @file hal.h
 * @brief Zmartify HAL - Hardware Abstraction Layer umbrella header
 *
 * Include this single header to gain access to all HAL subsystems.
 * GPIO numbers, I²C addresses, and peripheral config live here only.
 *
 * Hardware: ZIC-S3 Rev.B (ESP32-S3, 8 MB)
 * Architecture ref: MEP v5.0 Volume 2, Chapter 4
 */

#pragma once

#include "hal_gpio.h"
#include "hal_i2c.h"
#include "hal_relay.h"
#include "hal_flow.h"
#include "hal_pressure.h"
#include "hal_time.h"
#include "hal_storage.h"

/* ──────────────────────────────────────────────
 *  Shared HAL status / result type
 * ────────────────────────────────────────────── */

/**
 * @brief Common return codes for all HAL functions.
 *
 * Every HAL function returns hal_result_t.
 * Callers must check the result and decide how to respond.
 * The HAL never generates alarms or makes application decisions.
 */
typedef enum hal_result
{
    HAL_OK                 = 0,  /**< Operation succeeded */
    HAL_TIMEOUT            = 1,  /**< Operation timed out */
    HAL_INVALID_PARAMETER  = 2,  /**< Bad argument supplied */
    HAL_DEVICE_NOT_FOUND   = 3,  /**< Device did not respond */
    HAL_IO_ERROR           = 4,  /**< I/O error (bus, read/write) */
    HAL_BUSY               = 5,  /**< Resource in use */
    HAL_NOT_INITIALIZED    = 6,  /**< Module not yet initialised */
    HAL_OUT_OF_RANGE       = 7,  /**< Measurement outside valid range */
    HAL_OVERFLOW           = 8,  /**< Counter or buffer overflow */
} hal_result_t;

/* ──────────────────────────────────────────────
 *  Board-level pin constants  (ZIC-S3 Rev.B)
 *
 *  GPIO numbers shall ONLY appear in this header
 *  or inside hal_*.c implementation files.
 *  No other component may reference GPIO numbers.
 * ────────────────────────────────────────────── */

/* I²C Bus 0  ─  internal sensors */
#define ZIC_I2C_PORT           0
#define ZIC_I2C_SDA_PIN        8
#define ZIC_I2C_SCL_PIN        9
#define ZIC_I2C_FREQ_HZ        400000   /**< 400 kHz Fast Mode */

/* MCP23017 (GPIO expander → relay bank) */
#define ZIC_MCP23017_ADDR_0    0x20     /**< A2=0 A1=0 A0=0 */
#define ZIC_MCP23017_ADDR_1    0x21     /**< A2=0 A1=0 A0=1 */
#define ZIC_MCP23017_ADDR_2    0x22     /**< A2=0 A1=1 A0=0 */

/* ADS1115 (16-bit ADC) */
#define ZIC_ADS1115_ADDR       0x48     /**< ADDR pin → GND */

/* MCP9808 (temperature) */
#define ZIC_MCP9808_ADDR       0x18     /**< A2=A1=A0=0 */

/* Flow meter PCNT inputs (Hall-effect) */
#define ZIC_FLOW_PCNT_PIN_0    4        /**< Flow meter 0 */
#define ZIC_FLOW_PCNT_PIN_1    5        /**< Flow meter 1 */
#define ZIC_FLOW_PCNT_UNIT_0   0
#define ZIC_FLOW_PCNT_UNIT_1   1

/* Status LED */
#define ZIC_LED_STATUS_PIN     48       /**< Onboard RGB LED (WS2812-compatible) */

/* UART1 / RS-485 */
#define ZIC_RS485_TX_PIN       17
#define ZIC_RS485_RX_PIN       18
#define ZIC_RS485_DE_PIN       16       /**< DE/RE direction control */

/* Relay count */
#define ZIC_RELAY_COUNT        16       /**< Max concurrent relays */
#define ZIC_RELAY_ZONE_FIRST   0        /**< Relay 0 = Zone 1 */
#define ZIC_RELAY_MASTER_VALVE 15       /**< Relay 15 = Master Valve */

/* ──────────────────────────────────────────────
 *  HAL subsystem initialisation
 * ────────────────────────────────────────────── */

/**
 * @brief Initialise all HAL subsystems in dependency order.
 *
 * Calls each module's init function:
 *   GPIO → I²C → Storage → Relay → Flow → Pressure → Time
 *
 * Returns false on the first failure; the failing module is logged.
 *
 * @return true  All modules initialised successfully
 * @return false At least one module failed
 */
bool hal_init(void);
