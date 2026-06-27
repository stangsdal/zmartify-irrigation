#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

bool hmi_7b_bus_init(void);
bool hmi_7b_bus_probe(uint8_t addr);
bool hmi_7b_bus_write(uint8_t addr, const uint8_t *data, size_t len);
bool hmi_7b_bus_read(uint8_t addr, uint8_t *data, size_t len);
bool hmi_7b_bus_write_reg8(uint8_t addr, uint8_t reg, const uint8_t *data, size_t len);
bool hmi_7b_bus_read_reg16(uint8_t addr, uint16_t reg, uint8_t *data, size_t len);
bool hmi_7b_bus_write_reg16(uint8_t addr, uint16_t reg, const uint8_t *data, size_t len);
