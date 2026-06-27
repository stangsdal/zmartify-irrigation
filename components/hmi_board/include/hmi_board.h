#pragma once

#include <stdbool.h>

typedef struct
{
    bool backlight_enabled;
    bool touch_present;
    bool panel_ready;
} hmi_board_status_t;

bool hmi_board_init(void);
void hmi_board_get_status(hmi_board_status_t *out_status);
