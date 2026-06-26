/*
 * hal.c
 * HAL umbrella – initialises all subsystems in dependency order
 */

#include "hal.h"
#include "esp_log.h"

static const char *TAG = "hal";

bool zic_hal_init(void)
{
    ESP_LOGI(TAG, "Initialising Hardware Abstraction Layer...");

    /* 1. GPIO – first; other modules may use GPIO */
    if (hal_gpio_init() != HAL_OK)
    {
        ESP_LOGE(TAG, "hal_gpio_init FAILED");
        return false;
    }

    /* 2. I²C – required by relay, pressure modules */
    if (hal_i2c_init() != HAL_OK)
    {
        ESP_LOGE(TAG, "hal_i2c_init FAILED");
        return false;
    }

    /* 3. NVS storage */
    if (hal_storage_init() != HAL_OK)
    {
        ESP_LOGE(TAG, "hal_storage_init FAILED");
        return false;
    }

    /* 4. Relay driver (MCP23017 over I²C) */
    hal_result_t r = hal_relay_init();
    if (r != HAL_OK && r != HAL_DEVICE_NOT_FOUND)
    {
        ESP_LOGE(TAG, "hal_relay_init FAILED: %d", r);
        return false;
    }

    /* 5. Flow meter pulse counters */
    if (hal_flow_init() != HAL_OK)
    {
        ESP_LOGE(TAG, "hal_flow_init FAILED");
        return false;
    }

    /* 6. Pressure ADC (ADS1115 over I²C) */
    r = hal_pressure_init();
    if (r != HAL_OK && r != HAL_DEVICE_NOT_FOUND)
    {
        ESP_LOGE(TAG, "hal_pressure_init FAILED: %d", r);
        return false;
    }

    /* 7. Time / SNTP – requires Wi-Fi to sync, but initialise the module now */
    if (hal_time_init(NULL) != HAL_OK)
    {
        ESP_LOGE(TAG, "hal_time_init FAILED");
        return false;
    }

    ESP_LOGI(TAG, "HAL initialisation complete");
    return true;
}
