#pragma once

#include <stdbool.h>
#include <stdint.h>

#define HMI_7B_IOEXP_ADDR 0x24

bool hmi_7b_ioexp_init(void);
bool hmi_7b_ioexp_set_output_bit(uint8_t bit, bool high);
bool hmi_7b_ioexp_set_backlight(bool on);
bool hmi_7b_ioexp_set_backlight_pwm(uint8_t value);
bool hmi_7b_ioexp_lcd_reset_pulse(void);
bool hmi_7b_ioexp_touch_reset_pulse(void);
