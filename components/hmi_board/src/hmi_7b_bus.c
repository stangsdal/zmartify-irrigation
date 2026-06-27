#include "hmi_7b_bus.h"

#include "hal.h"

#include <string.h>

bool hmi_7b_bus_init(void)
{
    return hal_i2c_init() == HAL_OK;
}

bool hmi_7b_bus_probe(uint8_t addr)
{
    return hal_i2c_probe(addr);
}

bool hmi_7b_bus_write(uint8_t addr, const uint8_t *data, size_t len)
{
    return hal_i2c_write(addr, data, len) == HAL_OK;
}

bool hmi_7b_bus_read(uint8_t addr, uint8_t *data, size_t len)
{
    return hal_i2c_read(addr, data, len) == HAL_OK;
}

bool hmi_7b_bus_write_reg8(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len)
{
    return hal_i2c_write_reg(addr, reg, data, len) == HAL_OK;
}

bool hmi_7b_bus_read_reg16(uint8_t addr, uint16_t reg, uint8_t *data, size_t len)
{
    if (data == NULL || len == 0)
    {
        return false;
    }

    uint8_t reg_buf[2] = {
        (uint8_t)(reg >> 8),
        (uint8_t)(reg & 0xFF)
    };

    if (hal_i2c_write(addr, reg_buf, sizeof(reg_buf)) != HAL_OK)
    {
        return false;
    }

    return hal_i2c_read(addr, data, len) == HAL_OK;
}

bool hmi_7b_bus_write_reg16(uint8_t addr, uint16_t reg, const uint8_t *data, size_t len)
{
    if (data == NULL || len == 0 || len > 30)
    {
        return false;
    }

    uint8_t buf[32];
    buf[0] = (uint8_t)(reg >> 8);
    buf[1] = (uint8_t)(reg & 0xFF);
    memcpy(&buf[2], data, len);

    return hal_i2c_write(addr, buf, len + 2) == HAL_OK;
}
