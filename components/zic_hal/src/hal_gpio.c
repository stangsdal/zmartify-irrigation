/*
 * hal_gpio.c
 * GPIO HAL implementation for ZIC-S3 Rev.B
 */

#include "hal.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include <string.h>

static const char *TAG = "hal_gpio";
static bool s_initialized = false;

hal_result_t hal_gpio_init(void)
{
    if (s_initialized)
    {
        return HAL_OK;
    }

    /* Install the GPIO ISR service so individual handlers can be attached */
    esp_err_t err = gpio_install_isr_service(0);
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE)
    {
        ESP_LOGE(TAG, "Failed to install GPIO ISR service: %d", err);
        return HAL_IO_ERROR;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "GPIO HAL initialized");
    return HAL_OK;
}

hal_result_t hal_gpio_config(int pin, hal_gpio_dir_t dir, hal_gpio_pull_t pull)
{
    if (!s_initialized)
    {
        return HAL_NOT_INITIALIZED;
    }

    gpio_config_t cfg = {
        .pin_bit_mask = (1ULL << pin),
        .mode         = (dir == HAL_GPIO_DIR_OUTPUT) ? GPIO_MODE_OUTPUT : GPIO_MODE_INPUT,
        .pull_up_en   = (pull == HAL_GPIO_PULL_UP)   ? GPIO_PULLUP_ENABLE   : GPIO_PULLUP_DISABLE,
        .pull_down_en = (pull == HAL_GPIO_PULL_DOWN) ? GPIO_PULLDOWN_ENABLE : GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };

    if (gpio_config(&cfg) != ESP_OK)
    {
        ESP_LOGE(TAG, "gpio_config failed for pin %d", pin);
        return HAL_IO_ERROR;
    }

    return HAL_OK;
}

hal_result_t hal_gpio_write(int pin, bool level)
{
    if (!s_initialized)
    {
        return HAL_NOT_INITIALIZED;
    }
    gpio_set_level(pin, level ? 1 : 0);
    return HAL_OK;
}

hal_result_t hal_gpio_read(int pin, bool *level)
{
    if (!s_initialized || level == NULL)
    {
        return HAL_NOT_INITIALIZED;
    }
    *level = (gpio_get_level(pin) != 0);
    return HAL_OK;
}

hal_result_t hal_gpio_toggle(int pin)
{
    if (!s_initialized)
    {
        return HAL_NOT_INITIALIZED;
    }
    int cur = gpio_get_level(pin);
    gpio_set_level(pin, cur ? 0 : 1);
    return HAL_OK;
}

hal_result_t hal_gpio_attach_interrupt(int pin, hal_gpio_intr_t trigger,
                                        hal_gpio_isr_t isr, void *context)
{
    if (!s_initialized || isr == NULL)
    {
        return HAL_INVALID_PARAMETER;
    }

    gpio_int_type_t esp_intr;
    switch (trigger)
    {
        case HAL_GPIO_INTR_RISING_EDGE:  esp_intr = GPIO_INTR_POSEDGE; break;
        case HAL_GPIO_INTR_FALLING_EDGE: esp_intr = GPIO_INTR_NEGEDGE; break;
        case HAL_GPIO_INTR_ANY_EDGE:     esp_intr = GPIO_INTR_ANYEDGE; break;
        default:                         esp_intr = GPIO_INTR_DISABLE; break;
    }

    gpio_set_intr_type(pin, esp_intr);
    if (gpio_isr_handler_add(pin, (gpio_isr_t)isr, context) != ESP_OK)
    {
        ESP_LOGE(TAG, "Failed to attach ISR for pin %d", pin);
        return HAL_IO_ERROR;
    }

    return HAL_OK;
}

hal_result_t hal_gpio_detach_interrupt(int pin)
{
    if (!s_initialized)
    {
        return HAL_NOT_INITIALIZED;
    }
    gpio_isr_handler_remove(pin);
    gpio_set_intr_type(pin, GPIO_INTR_DISABLE);
    return HAL_OK;
}
