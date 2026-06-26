/**
 * @file hal_gpio.h
 * @brief GPIO HAL - Digital I/O abstraction
 *
 * All GPIO access must go through this interface.
 * GPIO pin numbers are defined in hal.h only.
 *
 * Architecture ref: MEP v5.0 Volume 2, Chapter 4, Section 4.5
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/* Forward declare hal_result_t (defined in hal.h) */
typedef enum hal_result hal_result_t;

/**
 * @brief GPIO direction
 */
typedef enum
{
    HAL_GPIO_DIR_INPUT  = 0,
    HAL_GPIO_DIR_OUTPUT = 1,
} hal_gpio_dir_t;

/**
 * @brief GPIO pull configuration
 */
typedef enum
{
    HAL_GPIO_PULL_NONE  = 0,
    HAL_GPIO_PULL_UP    = 1,
    HAL_GPIO_PULL_DOWN  = 2,
} hal_gpio_pull_t;

/**
 * @brief GPIO interrupt trigger mode
 */
typedef enum
{
    HAL_GPIO_INTR_DISABLE     = 0,
    HAL_GPIO_INTR_RISING_EDGE = 1,
    HAL_GPIO_INTR_FALLING_EDGE= 2,
    HAL_GPIO_INTR_ANY_EDGE    = 3,
} hal_gpio_intr_t;

/** Callback signature for GPIO interrupts */
typedef void (*hal_gpio_isr_t)(void *context);

/**
 * @brief Initialise the GPIO subsystem.
 *
 * Must be called once before any other hal_gpio_* function.
 * @return HAL_OK on success.
 */
hal_result_t hal_gpio_init(void);

/**
 * @brief Configure a GPIO pin direction and pull.
 */
hal_result_t hal_gpio_config(int pin, hal_gpio_dir_t dir, hal_gpio_pull_t pull);

/**
 * @brief Write a digital level to an output pin.
 *
 * @param pin   GPIO number
 * @param level true = high, false = low
 */
hal_result_t hal_gpio_write(int pin, bool level);

/**
 * @brief Read the current level of a GPIO pin.
 *
 * @param pin     GPIO number
 * @param level   Output: true = high, false = low
 */
hal_result_t hal_gpio_read(int pin, bool *level);

/**
 * @brief Toggle a GPIO output pin.
 */
hal_result_t hal_gpio_toggle(int pin);

/**
 * @brief Attach an interrupt handler to a GPIO input.
 *
 * @param pin      GPIO number
 * @param trigger  Edge trigger type
 * @param isr      Callback function (called from ISR context)
 * @param context  User-supplied context pointer passed to callback
 */
hal_result_t hal_gpio_attach_interrupt(int pin, hal_gpio_intr_t trigger,
                                       hal_gpio_isr_t isr, void *context);

/**
 * @brief Detach an interrupt handler from a GPIO pin.
 */
hal_result_t hal_gpio_detach_interrupt(int pin);
