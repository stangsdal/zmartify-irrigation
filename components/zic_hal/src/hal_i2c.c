/*
 * hal_i2c.c
 * I²C HAL implementation for ZIC-S3 Rev.B
 * Bus: I2C_NUM_0, SDA=8, SCL=9, 400 kHz
 */

#include "hal.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "driver/i2c_master.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "hal_i2c";

static i2c_master_bus_handle_t s_bus_handle = NULL;
static SemaphoreHandle_t       s_bus_mutex  = NULL;
static bool                    s_initialized = false;
static int64_t                 s_last_warn_us = 0;
static uint32_t                s_warn_suppressed = 0;

#define I2C_WARN_THROTTLE_US   (5 * 1000 * 1000)

static void log_i2c_error_throttled(const char *op, uint8_t addr, int reg, esp_err_t err)
{
    int64_t now_us = esp_timer_get_time();
    if ((now_us - s_last_warn_us) < I2C_WARN_THROTTLE_US)
    {
        s_warn_suppressed++;
        return;
    }

    if (reg >= 0)
    {
        ESP_LOGW(TAG,
                 "%s 0x%02X reg=0x%02X failed: %d (%lu suppressed)",
                 op,
                 addr,
                 (unsigned)reg,
                 err,
                 (unsigned long)s_warn_suppressed);
    }
    else
    {
        ESP_LOGW(TAG,
                 "%s 0x%02X failed: %d (%lu suppressed)",
                 op,
                 addr,
                 err,
                 (unsigned long)s_warn_suppressed);
    }

    s_last_warn_us = now_us;
    s_warn_suppressed = 0;
}

hal_result_t hal_i2c_init(void)
{
    if (s_initialized)
    {
        return HAL_OK;
    }

    s_bus_mutex = xSemaphoreCreateMutex();
    if (s_bus_mutex == NULL)
    {
        ESP_LOGE(TAG, "Failed to create I2C mutex");
        return HAL_IO_ERROR;
    }

    i2c_master_bus_config_t bus_cfg = {
        .i2c_port      = ZIC_I2C_PORT,
        .sda_io_num    = ZIC_I2C_SDA_PIN,
        .scl_io_num    = ZIC_I2C_SCL_PIN,
        .clk_source    = I2C_CLK_SRC_DEFAULT,
        .glitch_ignore_cnt = 7,
        .flags.enable_internal_pullup = true,
    };

    esp_err_t err = i2c_new_master_bus(&bus_cfg, &s_bus_handle);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "i2c_new_master_bus failed: %d", err);
        vSemaphoreDelete(s_bus_mutex);
        s_bus_mutex = NULL;
        return HAL_IO_ERROR;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "I2C HAL initialized (SDA=%d SCL=%d %d Hz)",
             ZIC_I2C_SDA_PIN, ZIC_I2C_SCL_PIN, ZIC_I2C_FREQ_HZ);
    return HAL_OK;
}

hal_result_t hal_i2c_deinit(void)
{
    if (!s_initialized)
    {
        return HAL_OK;
    }
    i2c_del_master_bus(s_bus_handle);
    s_bus_handle = NULL;
    vSemaphoreDelete(s_bus_mutex);
    s_bus_mutex = NULL;
    s_initialized = false;
    return HAL_OK;
}

bool hal_i2c_probe(uint8_t addr)
{
    if (!s_initialized)
    {
        return false;
    }
    esp_err_t err = i2c_master_probe(s_bus_handle, addr, HAL_I2C_TIMEOUT_MS);
    return (err == ESP_OK);
}

hal_result_t hal_i2c_write(uint8_t addr, const uint8_t *data, size_t len)
{
    if (!s_initialized)
    {
        return HAL_NOT_INITIALIZED;
    }
    if (data == NULL || len == 0)
    {
        return HAL_INVALID_PARAMETER;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = addr,
        .scl_speed_hz    = ZIC_I2C_FREQ_HZ,
    };
    i2c_master_dev_handle_t dev;

    xSemaphoreTake(s_bus_mutex, portMAX_DELAY);
    esp_err_t err = i2c_master_bus_add_device(s_bus_handle, &dev_cfg, &dev);
    if (err == ESP_OK)
    {
        err = i2c_master_transmit(dev, data, len, HAL_I2C_TIMEOUT_MS);
        i2c_master_bus_rm_device(dev);
    }
    xSemaphoreGive(s_bus_mutex);

    if (err != ESP_OK)
    {
        log_i2c_error_throttled("write", addr, -1, err);
        return HAL_IO_ERROR;
    }
    return HAL_OK;
}

hal_result_t hal_i2c_read(uint8_t addr, uint8_t *buf, size_t len)
{
    if (!s_initialized)
    {
        return HAL_NOT_INITIALIZED;
    }
    if (buf == NULL || len == 0)
    {
        return HAL_INVALID_PARAMETER;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = addr,
        .scl_speed_hz    = ZIC_I2C_FREQ_HZ,
    };
    i2c_master_dev_handle_t dev;

    xSemaphoreTake(s_bus_mutex, portMAX_DELAY);
    esp_err_t err = i2c_master_bus_add_device(s_bus_handle, &dev_cfg, &dev);
    if (err == ESP_OK)
    {
        err = i2c_master_receive(dev, buf, len, HAL_I2C_TIMEOUT_MS);
        i2c_master_bus_rm_device(dev);
    }
    xSemaphoreGive(s_bus_mutex);

    if (err != ESP_OK)
    {
        log_i2c_error_throttled("read", addr, -1, err);
        return HAL_IO_ERROR;
    }
    return HAL_OK;
}

hal_result_t hal_i2c_write_read(uint8_t addr, uint8_t reg,
                                  uint8_t *buf, size_t len)
{
    if (!s_initialized)
    {
        return HAL_NOT_INITIALIZED;
    }
    if (buf == NULL || len == 0)
    {
        return HAL_INVALID_PARAMETER;
    }

    i2c_device_config_t dev_cfg = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address  = addr,
        .scl_speed_hz    = ZIC_I2C_FREQ_HZ,
    };
    i2c_master_dev_handle_t dev;

    xSemaphoreTake(s_bus_mutex, portMAX_DELAY);
    esp_err_t err = i2c_master_bus_add_device(s_bus_handle, &dev_cfg, &dev);
    if (err == ESP_OK)
    {
        err = i2c_master_transmit_receive(dev, &reg, 1, buf, len,
                                          HAL_I2C_TIMEOUT_MS);
        i2c_master_bus_rm_device(dev);
    }
    xSemaphoreGive(s_bus_mutex);

    if (err != ESP_OK)
    {
        log_i2c_error_throttled("write_read", addr, reg, err);
        return HAL_IO_ERROR;
    }
    return HAL_OK;
}

hal_result_t hal_i2c_write_reg(uint8_t addr, uint8_t reg,
                                 const uint8_t *data, size_t len)
{
    if (!s_initialized)
    {
        return HAL_NOT_INITIALIZED;
    }
    if (data == NULL || len == 0 || len > 64)
    {
        return HAL_INVALID_PARAMETER;
    }

    /* Build reg + data into a single buffer (max 65 bytes) */
    uint8_t buf[65];
    buf[0] = reg;
    memcpy(&buf[1], data, len);

    return hal_i2c_write(addr, buf, len + 1);
}

hal_result_t hal_i2c_recover(void)
{
    if (!s_initialized)
    {
        return HAL_NOT_INITIALIZED;
    }
    ESP_LOGW(TAG, "Attempting I2C bus recovery");
    /* Re-init the bus */
    hal_i2c_deinit();
    return hal_i2c_init();
}
