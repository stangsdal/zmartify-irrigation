/**
 * @file hal_i2c.h
 * @brief I²C HAL - I²C bus master abstraction
 *
 * Wraps the ESP-IDF I²C driver.
 * Supports device detection, read/write, and error recovery.
 *
 * Architecture ref: MEP v5.0 Volume 2, Chapter 4, Section 4.6
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef enum hal_result hal_result_t;

/** Default I²C timeout (ms) */
#define HAL_I2C_TIMEOUT_MS   100

/**
 * @brief Initialise the I²C bus master.
 *
 * Configures port, SDA/SCL pins, and frequency per hal.h constants.
 * @return HAL_OK on success.
 */
hal_result_t hal_i2c_init(void);

/**
 * @brief Deinitialise the I²C bus (call during shutdown only).
 */
hal_result_t hal_i2c_deinit(void);

/**
 * @brief Probe for a device at the given 7-bit address.
 *
 * Sends a zero-length write and checks for ACK.
 * @return true if the device acknowledged.
 */
bool hal_i2c_probe(uint8_t addr);

/**
 * @brief Write bytes to an I²C device.
 *
 * @param addr    7-bit device address
 * @param data    Buffer to send
 * @param len     Number of bytes to send
 * @return HAL_OK on success, HAL_IO_ERROR on NACK.
 */
hal_result_t hal_i2c_write(uint8_t addr, const uint8_t *data, size_t len);

/**
 * @brief Read bytes from an I²C device.
 *
 * @param addr    7-bit device address
 * @param buf     Receive buffer
 * @param len     Number of bytes to read
 * @return HAL_OK on success, HAL_IO_ERROR on NACK.
 */
hal_result_t hal_i2c_read(uint8_t addr, uint8_t *buf, size_t len);

/**
 * @brief Write a register address then read the response (combined transaction).
 *
 * @param addr    7-bit device address
 * @param reg     Register/command byte
 * @param buf     Receive buffer
 * @param len     Number of bytes to read
 */
hal_result_t hal_i2c_write_read(uint8_t addr, uint8_t reg,
                                 uint8_t *buf, size_t len);

/**
 * @brief Write register address then write data (register write).
 *
 * @param addr    7-bit device address
 * @param reg     Register/command byte
 * @param data    Data to write
 * @param len     Number of data bytes
 */
hal_result_t hal_i2c_write_reg(uint8_t addr, uint8_t reg,
                                const uint8_t *data, size_t len);

/**
 * @brief Attempt to recover the bus if it is stuck.
 *
 * Issues up to 9 clock pulses to release a stuck device.
 */
hal_result_t hal_i2c_recover(void);
