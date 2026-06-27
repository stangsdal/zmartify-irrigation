#pragma once

#include <stdbool.h>
#include <stdint.h>

bool hmi_7b_touch_detect(void);
bool hmi_7b_touch_init(void);
bool hmi_7b_touch_read(uint16_t *x, uint16_t *y, bool *pressed);
