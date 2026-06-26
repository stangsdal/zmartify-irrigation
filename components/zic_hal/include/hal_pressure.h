/**
 * @file hal_pressure.h
 * @brief Pressure HAL - ADS1115 16-bit ADC interface
 *
 * Reads raw ADC counts and converts to calibrated voltage.
 * Pressure conversion from voltage to bar/PSI happens in the Pressure Manager.
 *
 * Architecture ref: MEP v5.0 Volume 2, Chapter 4, Section 4.9
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef enum hal_result hal_result_t;

/**
 * @brief ADS1115 channel selection
 *
 * Single-ended inputs relative to GND.
 */
typedef enum
{
    HAL_ADC_CHANNEL_0 = 0,  /**< Pressure sensor input */
    HAL_ADC_CHANNEL_1 = 1,  /**< Temperature / spare */
    HAL_ADC_CHANNEL_2 = 2,  /**< Spare                */
    HAL_ADC_CHANNEL_3 = 3,  /**< Spare                */
} hal_adc_channel_t;

/**
 * @brief ADS1115 programmable-gain amplifier setting
 */
typedef enum
{
    HAL_ADC_PGA_6V144  = 0,  /**< ±6.144 V  LSB = 187.5 µV */
    HAL_ADC_PGA_4V096  = 1,  /**< ±4.096 V  LSB = 125.0 µV */
    HAL_ADC_PGA_2V048  = 2,  /**< ±2.048 V  LSB =  62.5 µV (default) */
    HAL_ADC_PGA_1V024  = 3,  /**< ±1.024 V  LSB =  31.25 µV */
    HAL_ADC_PGA_0V512  = 4,  /**< ±0.512 V  LSB =  15.625 µV */
    HAL_ADC_PGA_0V256  = 5,  /**< ±0.256 V  LSB =   7.8125 µV */
} hal_adc_pga_t;

/**
 * @brief Initialise the ADS1115 ADC.
 *
 * Probes the device on the I²C bus and applies default configuration.
 * @return HAL_OK on success; HAL_DEVICE_NOT_FOUND if ADS1115 absent.
 */
hal_result_t hal_pressure_init(void);

/**
 * @brief Read a raw 16-bit signed ADC value.
 *
 * Triggers a single-shot conversion (blocking, ~2 ms).
 *
 * @param channel  ADC channel to read
 * @param pga      Gain setting for this measurement
 * @param raw      Output: signed 16-bit raw ADC count
 * @return HAL_OK on success.
 */
hal_result_t hal_pressure_read_raw(hal_adc_channel_t channel,
                                    hal_adc_pga_t pga,
                                    int16_t *raw);

/**
 * @brief Read a channel and return the calibrated voltage in millivolts.
 *
 * @param channel    ADC channel
 * @param pga        Gain setting
 * @param voltage_mv Output: voltage in millivolts
 * @return HAL_OK on success.
 */
hal_result_t hal_pressure_read_voltage(hal_adc_channel_t channel,
                                        hal_adc_pga_t pga,
                                        int32_t *voltage_mv);
