#pragma once

#include <stdbool.h>

#include "hmi_controller.h"

typedef struct
{
    bool backlight_enabled;
    bool touch_present;
    bool panel_ready;
} hmi_board_status_t;

typedef bool (*hmi_snapshot_fn)(void *context, hmi_view_model_t *view_model);

typedef struct
{
    hmi_snapshot_fn snapshot;
    hmi_dispatch_fn dispatch;
    void *context;
} hmi_board_bindings_t;

bool hmi_board_init(void);
bool hmi_board_bind(const hmi_board_bindings_t *bindings);
void hmi_board_get_status(hmi_board_status_t *out_status);
