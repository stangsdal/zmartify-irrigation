/*
 * hal_relay.c
 * Relay HAL – MCP23017 GPIO expander for valve/relay control
 *
 * MCP23017 register map (IOCON.BANK=0, default):
 *   0x00 IODIRA   – I/O direction port A (0=output)
 *   0x01 IODIRB   – I/O direction port B
 *   0x12 GPIOA    – GPIO port A latch
 *   0x13 GPIOB    – GPIO port B latch
 *
 * Relay assignments:
 *   GPIOA bits 0-7 → Relays  0-7
 *   GPIOB bits 0-7 → Relays  8-15
 *
 * Logic: relay energised → GPIO HIGH → ULN2803A → solenoid ON
 */

#include "hal.h"
#include "esp_log.h"
#include <string.h>

static const char *TAG = "hal_relay";

/* MCP23017 registers */
#define MCP23017_IODIRA   0x00
#define MCP23017_IODIRB   0x01
#define MCP23017_GPIOA    0x12
#define MCP23017_GPIOB    0x13

/* Shadow register: one byte per MCP23017 port. */
static uint8_t s_relay_shadow[2] = {0, 0};
static bool    s_initialized = false;

static hal_result_t mcp_write_port(uint8_t reg, uint8_t value)
{
    uint8_t data = value;
    return hal_i2c_write_reg(ZIC_MCP23017_ADDR_0, reg, &data, 1);
}

static hal_result_t mcp_configure_outputs(void)
{
    uint8_t zero = 0x00;  /* all outputs */
    if (hal_i2c_write_reg(ZIC_MCP23017_ADDR_0, MCP23017_IODIRA, &zero, 1) != HAL_OK)
    {
        return HAL_IO_ERROR;
    }
    return hal_i2c_write_reg(ZIC_MCP23017_ADDR_0, MCP23017_IODIRB, &zero, 1);
}

hal_result_t hal_relay_init(void)
{
    if (s_initialized)
    {
        return HAL_OK;
    }

    if (!hal_i2c_probe(ZIC_MCP23017_ADDR_0))
    {
        ESP_LOGW(TAG, "MCP23017 at 0x%02X not found", ZIC_MCP23017_ADDR_0);
        return HAL_DEVICE_NOT_FOUND;
    }

    hal_result_t result = mcp_configure_outputs();
    if (result != HAL_OK)
    {
        ESP_LOGE(TAG, "Failed to configure MCP23017 outputs");
        return result;
    }

    s_relay_shadow[0] = 0x00;
    s_relay_shadow[1] = 0x00;
    result = hal_relay_all_off();
    if (result != HAL_OK)
    {
        ESP_LOGE(TAG, "Failed to clear MCP23017 outputs");
        return result;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "MCP23017 at 0x%02X ready - %d relays", ZIC_MCP23017_ADDR_0, HAL_RELAY_COUNT);
    return HAL_OK;
}

hal_result_t hal_relay_on(uint8_t relay)
{
    if (!s_initialized)
    {
        return HAL_NOT_INITIALIZED;
    }
    if (relay >= HAL_RELAY_COUNT)
    {
        return HAL_INVALID_PARAMETER;
    }

    uint8_t port  = relay / 8;
    uint8_t bit   = relay % 8;
    static const uint8_t gpio_regs[2] = {MCP23017_GPIOA, MCP23017_GPIOB};

    s_relay_shadow[port] |= (1u << bit);
    return mcp_write_port(gpio_regs[port], s_relay_shadow[port]);
}

hal_result_t hal_relay_off(uint8_t relay)
{
    if (!s_initialized)
    {
        return HAL_NOT_INITIALIZED;
    }
    if (relay >= HAL_RELAY_COUNT)
    {
        return HAL_INVALID_PARAMETER;
    }

    uint8_t port  = relay / 8;
    uint8_t bit   = relay % 8;
    static const uint8_t gpio_regs[2] = {MCP23017_GPIOA, MCP23017_GPIOB};

    s_relay_shadow[port] &= ~(1u << bit);
    return mcp_write_port(gpio_regs[port], s_relay_shadow[port]);
}

bool hal_relay_get(uint8_t relay)
{
    if (relay >= HAL_RELAY_COUNT)
    {
        return false;
    }
    uint8_t chip = relay / 8;
    uint8_t bit  = relay % 8;
    return (s_relay_shadow[chip] & (1u << bit)) != 0;
}

hal_result_t hal_relay_all_off(void)
{
    static const uint8_t gpio_regs[2] = {MCP23017_GPIOA, MCP23017_GPIOB};
    hal_result_t result = HAL_OK;

    for (int i = 0; i < 2; i++)
    {
        s_relay_shadow[i] = 0x00;
        hal_result_t r = mcp_write_port(gpio_regs[i], 0x00);
        if (r != HAL_OK)
        {
            ESP_LOGE(TAG, "hal_relay_all_off: port %c write failed", 'A' + i);
            result = r;
        }
    }
    return result;
}
