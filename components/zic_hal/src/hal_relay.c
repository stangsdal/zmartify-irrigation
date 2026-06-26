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
 *   Chip 0 GPIOA bits 0-7 → Relays  0-7  (Zones 1-8)
 *   Chip 1 GPIOA bits 0-7 → Relays  8-15 (Zones 9-15, Master=15)
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

/* Shadow register: one byte per chip (8 relays per chip on port A) */
static uint8_t s_relay_shadow[2] = {0, 0};   /* chip 0, chip 1 */
static bool    s_initialized = false;

/** Write port A output latch for a given MCP23017 */
static hal_result_t mcp_write_gpioa(uint8_t chip_addr, uint8_t value)
{
    uint8_t data = value;
    return hal_i2c_write_reg(chip_addr, MCP23017_GPIOA, &data, 1);
}

/** Configure all port-A pins as outputs */
static hal_result_t mcp_configure_outputs(uint8_t chip_addr)
{
    uint8_t zero = 0x00;  /* all outputs */
    return hal_i2c_write_reg(chip_addr, MCP23017_IODIRA, &zero, 1);
}

hal_result_t hal_relay_init(void)
{
    if (s_initialized)
    {
        return HAL_OK;
    }

    static const uint8_t chips[2] = {ZIC_MCP23017_ADDR_0, ZIC_MCP23017_ADDR_1};

    for (int i = 0; i < 2; i++)
    {
        if (!hal_i2c_probe(chips[i]))
        {
            ESP_LOGW(TAG, "MCP23017 at 0x%02X not found – relay bank %d unavailable",
                     chips[i], i);
            /* Continue – may be running without hardware (e.g. development) */
            continue;
        }

        /* All outputs, start de-energised */
        hal_result_t r = mcp_configure_outputs(chips[i]);
        if (r != HAL_OK)
        {
            ESP_LOGE(TAG, "Failed to configure MCP23017 0x%02X outputs", chips[i]);
            return r;
        }

        s_relay_shadow[i] = 0x00;
        r = mcp_write_gpioa(chips[i], s_relay_shadow[i]);
        if (r != HAL_OK)
        {
            ESP_LOGE(TAG, "Failed to clear MCP23017 0x%02X outputs", chips[i]);
            return r;
        }

        ESP_LOGI(TAG, "MCP23017 chip %d (0x%02X) ready", i, chips[i]);
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Relay HAL initialized – %d relays", HAL_RELAY_COUNT);
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

    uint8_t chip  = relay / 8;
    uint8_t bit   = relay % 8;
    static const uint8_t chip_addrs[2] = {ZIC_MCP23017_ADDR_0, ZIC_MCP23017_ADDR_1};

    s_relay_shadow[chip] |= (1u << bit);
    return mcp_write_gpioa(chip_addrs[chip], s_relay_shadow[chip]);
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

    uint8_t chip  = relay / 8;
    uint8_t bit   = relay % 8;
    static const uint8_t chip_addrs[2] = {ZIC_MCP23017_ADDR_0, ZIC_MCP23017_ADDR_1};

    s_relay_shadow[chip] &= ~(1u << bit);
    return mcp_write_gpioa(chip_addrs[chip], s_relay_shadow[chip]);
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
    static const uint8_t chip_addrs[2] = {ZIC_MCP23017_ADDR_0, ZIC_MCP23017_ADDR_1};
    hal_result_t result = HAL_OK;

    for (int i = 0; i < 2; i++)
    {
        s_relay_shadow[i] = 0x00;
        hal_result_t r = mcp_write_gpioa(chip_addrs[i], 0x00);
        if (r != HAL_OK)
        {
            ESP_LOGE(TAG, "hal_relay_all_off: chip %d write failed", i);
            result = r;
        }
    }
    return result;
}
