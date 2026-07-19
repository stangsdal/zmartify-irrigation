#pragma once

#include <stdbool.h>

#include "esp_lcd_panel_rgb.h"
#include "esp_lcd_panel_ops.h"

#define HMI_7B_LCD_H_RES 1024
#define HMI_7B_LCD_V_RES 600

typedef struct
{
	bool hsync_idle_low;
	bool vsync_idle_low;
	bool de_idle_high;
	bool pclk_active_neg;
	bool pclk_idle_high;
} hmi_7b_rgb_flags_t;

bool hmi_7b_rgb_init(esp_lcd_panel_handle_t *out_panel);
bool hmi_7b_rgb_init_with_flags(esp_lcd_panel_handle_t *out_panel, const hmi_7b_rgb_flags_t *flags);
bool hmi_7b_rgb_register_event_callbacks(esp_lcd_panel_handle_t panel,
										 const esp_lcd_rgb_panel_event_callbacks_t *callbacks,
										 void *user_ctx);
bool hmi_7b_rgb_deinit(esp_lcd_panel_handle_t panel);
