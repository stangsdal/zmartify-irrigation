/*
 * hal_pressure.c
 * Pressure HAL – ADS1115 16-bit I²C ADC
 *
 * ADS1115 register map:
 *   0x00  Conversion register  (16-bit signed result)
 *   0x01  Config register      (16-bit control)
 *
 * Config register bits used:
 *   [15]    OS = 1       → start single-shot conversion
 *   [14:12] MUX          → channel selection
 *   [11:9]  PGA          → gain setting
 *   [8]     MODE = 1     → single-shot mode
 *   [7:5]   DR = 100     → 128 SPS (fast enough, 7.8 ms/sample)
 *   [4:0]   comparator disabled (default)
 */

#include "hal.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "hal_pressure";

/* ADS1115 I²C address (ADDR→GND = 0x48) */
#define ADS1115_ADDR         ZIC_ADS1115_ADDR

/* ADS1115 register addresses */
#define ADS1115_REG_CONV     0x00
#define ADS1115_REG_CONFIG   0x01

/* Config masks */
#define ADS1115_OS_SINGLE    0x8000u   /* bit15: start one-shot */
#define ADS1115_MODE_SINGLE  0x0100u   /* bit8: single-shot mode */
#define ADS1115_DR_128SPS    0x0080u   /* bits[7:5] = 100 */

/* MUX values for single-ended channels (AINx vs GND) */
static const uint16_t s_mux_table[4] = {
    0x4000u,   /* AIN0 */
    0x5000u,   /* AIN1 */
    0x6000u,   /* AIN2 */
    0x7000u,   /* AIN3 */
};

/* PGA full-scale voltage in µV */
static const int32_t s_pga_lsb_uv[6] = {
    187500,  /* ±6.144 V  → LSB = 187.5 µV */
    125000,  /* ±4.096 V  → LSB = 125.0 µV */
     62500,  /* ±2.048 V  → LSB =  62.5 µV */
     31250,  /* ±1.024 V  → LSB =  31.25 µV (stored as integer: 31250/1000 = 31) */
     15625,  /* ±0.512 V */
      7813,  /* ±0.256 V */
};

static bool s_initialized = false;

hal_result_t hal_pressure_init(void)
{
    if (s_initialized)
    {
        return HAL_OK;
    }

    if (!hal_i2c_probe(ADS1115_ADDR))
    {
        ESP_LOGW(TAG, "ADS1115 not found at 0x%02X", ADS1115_ADDR);
        /* Continue – allow firmware to boot without sensor present */
        s_initialized = true;
        return HAL_DEVICE_NOT_FOUND;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "ADS1115 found at 0x%02X", ADS1115_ADDR);
    return HAL_OK;
}

hal_result_t hal_pressure_read_raw(hal_adc_channel_t channel,
                                    hal_adc_pga_t pga,
                                    int16_t *raw)
{
    if (!s_initialized)
    {
        return HAL_NOT_INITIALIZED;
    }
    if (channel >= 4 || pga >= 6 || raw == NULL)
    {
        return HAL_INVALID_PARAMETER;
    }

    /* Build config word */
    uint16_t config = ADS1115_OS_SINGLE
                    | s_mux_table[channel]
                    | ((uint16_t)pga << 9)
                    | ADS1115_MODE_SINGLE
                    | ADS1115_DR_128SPS;

    uint8_t cfg_bytes[2] = {
        (uint8_t)(config >> 8),
        (uint8_t)(config & 0xFF),
    };

    /* Write config → trigger conversion */
    hal_result_t r = hal_i2c_write_reg(ADS1115_ADDR, ADS1115_REG_CONFIG,
                                        cfg_bytes, 2);
    if (r != HAL_OK)
    {
        return r;
    }

    /* Wait for conversion to complete (~8 ms for 128 SPS) */
    vTaskDelay(pdMS_TO_TICKS(10));

    /* Read 2-byte conversion result */
    uint8_t result[2] = {0, 0};
    r = hal_i2c_write_read(ADS1115_ADDR, ADS1115_REG_CONV, result, 2);
    if (r != HAL_OK)
    {
        return r;
    }

    *raw = (int16_t)((result[0] << 8) | result[1]);
    return HAL_OK;
}

hal_result_t hal_pressure_read_voltage(hal_adc_channel_t channel,
                                         hal_adc_pga_t pga,
                                         int32_t *voltage_mv)
{
    if (voltage_mv == NULL)
    {
        return HAL_INVALID_PARAMETER;
    }

    int16_t raw = 0;
    hal_result_t r = hal_pressure_read_raw(channel, pga, &raw);
    if (r != HAL_OK)
    {
        return r;
    }

    /* voltage_mv = raw * LSB_µV / 1000 */
    /* Use int64 to avoid overflow with large LSB values */
    int64_t uv = (int64_t)raw * s_pga_lsb_uv[pga];
    *voltage_mv = (int32_t)(uv / 1000000LL);

    return HAL_OK;
}
