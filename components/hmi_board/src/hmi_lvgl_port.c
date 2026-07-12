#include "esp_log.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_panel_rgb.h"
#include "esp_timer.h"
#include "esp_heap_caps.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "hmi_7b_rgb.h"
#include "lvgl.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static const char *TAG = "hmi_lvgl";

#define LCD_H_RES                      HMI_7B_LCD_H_RES
#define LCD_V_RES                      HMI_7B_LCD_V_RES

#define LVGL_TICK_MS                   2
#define LVGL_TASK_MIN_DELAY_MS         2
#define LVGL_TASK_MAX_DELAY_MS         16
#define LVGL_DRAW_LINES                40
#define STATUS_UPDATE_PERIOD_MS         1000
#define HMI_SCREEN_COUNT                5
#define HMI_NAV_X                       24
#define HMI_NAV_Y                       112
#define HMI_NAV_W                       (LCD_H_RES - 48)
#define HMI_NAV_H                       48
#define HMI_NAV_GAP                     8
#define HMI_NAV_BUTTON_Y_IN_NAV         8
#define HMI_NAV_BUTTON_H                32

/* Optional RGB bypass mode for low-level panel debugging (disabled by default). */
#define HMI_RGB_BYPASS_DIAG_MODE       0
#define HMI_RGB_PROFILE_SWEEP_MODE     0

typedef struct
{
    int16_t x;
    int16_t y;
    bool pressed;
    bool valid;
} hmi_touch_state_t;

typedef enum
{
    HMI_SCREEN_DASHBOARD = 0,
    HMI_SCREEN_IRRIGATION,
    HMI_SCREEN_WEATHER,
    HMI_SCREEN_HYDRAULICS,
    HMI_SCREEN_SETTINGS,
} hmi_screen_t;

static esp_lcd_panel_handle_t s_panel = NULL;
static SemaphoreHandle_t s_lvgl_mutex = NULL;
static TaskHandle_t s_lvgl_task = NULL;
static esp_timer_handle_t s_tick_timer = NULL;

static lv_obj_t *s_label_title = NULL;
static lv_obj_t *s_label_subtitle = NULL;
static lv_obj_t *s_label_uptime = NULL;
static lv_obj_t *s_label_system = NULL;
static lv_obj_t *s_label_zone = NULL;
static lv_obj_t *s_label_touch = NULL;
static lv_obj_t *s_label_perf = NULL;
static lv_obj_t *s_label_note = NULL;
static lv_obj_t *s_content_root = NULL;
static lv_obj_t *s_nav_buttons[HMI_SCREEN_COUNT] = {0};
static lv_obj_t *s_screen_cards[HMI_SCREEN_COUNT] = {0};
static hmi_screen_t s_active_screen = HMI_SCREEN_DASHBOARD;
static volatile bool s_screen_change_pending = false;
static volatile hmi_screen_t s_pending_screen = HMI_SCREEN_DASHBOARD;

static volatile uint32_t s_flush_count = 0;
static uint32_t s_last_flush_count = 0;
static int64_t s_last_perf_us = 0;
static int64_t s_last_status_update_us = 0;
static hmi_touch_state_t s_last_touch = {0};
static bool s_touch_was_pressed = false;

static lv_color_t *s_draw_buf_1 = NULL;
static lv_color_t *s_draw_buf_2 = NULL;

static const char *const s_screen_titles[HMI_SCREEN_COUNT] = {
    "Dashboard",
    "Irrigation",
    "Weather",
    "Hydraulics",
    "Settings",
};

static const char *screen_summary(hmi_screen_t screen)
{
    switch (screen)
    {
        case HMI_SCREEN_DASHBOARD:
            return "Operational overview and quick actions";
        case HMI_SCREEN_IRRIGATION:
            return "Programs, zones, runtime, and manual watering";
        case HMI_SCREEN_WEATHER:
            return "Weather summary, alerts, and irrigation holds";
        case HMI_SCREEN_HYDRAULICS:
            return "Flow, pressure, health, and diagnostics";
        case HMI_SCREEN_SETTINGS:
            return "Controller configuration and maintenance";
        default:
            return "Operational overview and quick actions";
    }
}

static void update_nav_highlight(void);
static void rebuild_active_screen(void);
static lv_obj_t *create_screen_body(lv_obj_t *parent, hmi_screen_t screen);
static void create_all_screen_bodies(lv_obj_t *parent);
static void process_nav_tap(int16_t x, int16_t y);

static void set_label_text_if_changed(lv_obj_t *label, const char *text)
{
    if (label == NULL || text == NULL)
    {
        return;
    }

    const char *current = lv_label_get_text(label);
    if (current != NULL && strcmp(current, text) == 0)
    {
        return;
    }

    lv_label_set_text(label, text);
}

#if HMI_RGB_BYPASS_DIAG_MODE
static TaskHandle_t s_rgb_diag_task = NULL;
static uint16_t *s_rgb_diag_frame = NULL;

typedef struct
{
    const char *name;
    hmi_7b_rgb_flags_t flags;
} rgb_diag_profile_t;

static const rgb_diag_profile_t s_rgb_profiles[] = {
    {"Waveshare pclk_neg", {.pclk_active_neg = true}},
    {"hsync low + pclk_neg", {.hsync_idle_low = true, .pclk_active_neg = true}},
    {"vsync low + pclk_neg", {.vsync_idle_low = true, .pclk_active_neg = true}},
    {"hsync/vsync low + pclk_neg", {.hsync_idle_low = true, .vsync_idle_low = true, .pclk_active_neg = true}},
    {"pclk idle high + pclk_neg", {.pclk_active_neg = true, .pclk_idle_high = true}},
    {"de idle high + pclk_neg", {.de_idle_high = true, .pclk_active_neg = true}},
    {"pclk positive edge", {.pclk_active_neg = false}},
    {"hsync/vsync low + pclk pos", {.hsync_idle_low = true, .vsync_idle_low = true, .pclk_active_neg = false}},
};
#endif

extern bool hmi_touch_init(void);
extern bool hmi_touch_read(uint16_t *x, uint16_t *y, bool *pressed);

static inline bool lvgl_lock(TickType_t timeout_ticks)
{
    return (s_lvgl_mutex != NULL) && (xSemaphoreTakeRecursive(s_lvgl_mutex, timeout_ticks) == pdTRUE);
}

static inline void lvgl_unlock(void)
{
    if (s_lvgl_mutex != NULL)
    {
        xSemaphoreGiveRecursive(s_lvgl_mutex);
    }
}

static void lvgl_tick_cb(void *arg)
{
    (void)arg;
    lv_tick_inc(LVGL_TICK_MS);
}

static void clamp_touch_to_panel(int16_t *x, int16_t *y)
{
    if (*x < 0)
    {
        *x = 0;
    }
    if (*y < 0)
    {
        *y = 0;
    }
    if (*x >= LCD_H_RES)
    {
        *x = LCD_H_RES - 1;
    }
    if (*y >= LCD_V_RES)
    {
        *y = LCD_V_RES - 1;
    }
}

static void update_nav_highlight(void)
{
    for (size_t i = 0; i < HMI_SCREEN_COUNT; ++i)
    {
        if (s_nav_buttons[i] == NULL)
        {
            continue;
        }

        lv_obj_t *label = lv_obj_get_child(s_nav_buttons[i], 0);
        if ((hmi_screen_t)i == s_active_screen)
        {
            lv_obj_set_style_bg_color(s_nav_buttons[i], lv_color_hex(0x1D3557), 0);
            lv_obj_set_style_border_color(s_nav_buttons[i], lv_color_hex(0x1D3557), 0);
            if (label != NULL)
            {
                lv_obj_set_style_text_color(label, lv_color_hex(0xF1FAEE), 0);
            }
        }
        else
        {
            lv_obj_set_style_bg_color(s_nav_buttons[i], lv_color_hex(0xE8EEF5), 0);
            lv_obj_set_style_border_color(s_nav_buttons[i], lv_color_hex(0xC7D6E3), 0);
            if (label != NULL)
            {
                lv_obj_set_style_text_color(label, lv_color_hex(0x1D3557), 0);
            }
        }
    }

    set_label_text_if_changed(s_label_subtitle, s_screen_titles[s_active_screen]);
}

static void rebuild_active_screen(void)
{
    if (s_content_root == NULL)
    {
        return;
    }

    for (size_t i = 0; i < HMI_SCREEN_COUNT; ++i)
    {
        if (s_screen_cards[i] == NULL)
        {
            continue;
        }

        if ((hmi_screen_t)i == s_active_screen)
        {
            lv_obj_clear_flag(s_screen_cards[i], LV_OBJ_FLAG_HIDDEN);
        }
        else
        {
            lv_obj_add_flag(s_screen_cards[i], LV_OBJ_FLAG_HIDDEN);
        }
    }

    update_nav_highlight();
}

static void apply_pending_screen_change(void)
{
    if (!s_screen_change_pending)
    {
        return;
    }

    s_screen_change_pending = false;
    s_active_screen = s_pending_screen;
    rebuild_active_screen();
}

static void process_nav_tap(int16_t x, int16_t y)
{
    const int nav_button_y0 = HMI_NAV_Y + HMI_NAV_BUTTON_Y_IN_NAV;
    const int nav_button_y1 = nav_button_y0 + HMI_NAV_BUTTON_H;
    if (y < nav_button_y0 || y >= nav_button_y1)
    {
        return;
    }

    const int nav_button_w = (HMI_NAV_W - (HMI_NAV_GAP * 6)) / HMI_SCREEN_COUNT;
    int x0 = HMI_NAV_X + HMI_NAV_GAP;
    for (size_t i = 0; i < HMI_SCREEN_COUNT; ++i)
    {
        int x1 = x0 + nav_button_w;
        if (x >= x0 && x < x1)
        {
            hmi_screen_t next_screen = (hmi_screen_t)i;
            if (next_screen != s_active_screen)
            {
                s_pending_screen = next_screen;
                s_screen_change_pending = true;
                ESP_LOGI(TAG, "screen change requested: %s", s_screen_titles[next_screen]);
            }
            return;
        }
        x0 = x1 + HMI_NAV_GAP;
    }
}

static lv_obj_t *create_screen_body(lv_obj_t *parent, hmi_screen_t screen)
{
    lv_obj_t *card = lv_obj_create(parent);
    lv_obj_set_size(card, LCD_H_RES - 72, 312);
    lv_obj_align(card, LV_ALIGN_TOP_MID, 0, 0);
    lv_obj_set_style_bg_color(card, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(card, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(card, lv_color_hex(0xC7D6E3), 0);
    lv_obj_set_style_border_width(card, 2, 0);
    lv_obj_set_style_radius(card, 18, 0);
    lv_obj_set_style_shadow_width(card, 0, 0);
    lv_obj_set_style_pad_all(card, 0, 0);
    lv_obj_clear_flag(card, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *title = lv_label_create(card);
    lv_label_set_text(title, s_screen_titles[screen]);
    lv_obj_set_style_text_color(title, lv_color_hex(0x1D3557), 0);
    lv_obj_align(title, LV_ALIGN_TOP_LEFT, 18, 14);

    lv_obj_t *summary = lv_label_create(card);
    lv_label_set_long_mode(summary, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(summary, LCD_H_RES - 120);
    lv_label_set_text(summary, screen_summary(screen));
    lv_obj_set_style_text_color(summary, lv_color_hex(0x457B9D), 0);
    lv_obj_align(summary, LV_ALIGN_TOP_LEFT, 18, 44);

    const char *lines[6] = {0};
    size_t line_count = 0;

    switch (screen)
    {
        case HMI_SCREEN_DASHBOARD:
            lines[0] = "Status: Idle";
            lines[1] = "Weather: 23.8 C  |  Humidity: 48%  |  ET: 2.1 mm";
            lines[2] = "Water: Flow 24.4 L/min  |  Pressure 3.48 bar";
            lines[3] = "Quick actions: Start  Stop  Rain delay  Manual zone  Alarms";
            line_count = 4;
            break;
        case HMI_SCREEN_IRRIGATION:
            lines[0] = "Current program: Program A";
            lines[1] = "Current zone: Zone 1";
            lines[2] = "Remaining runtime: 18 min";
            lines[3] = "Flow: 24.4 L/min";
            lines[4] = "Pressure: 3.48 bar";
            lines[5] = "Water used: 12.6 L";
            line_count = 6;
            break;
        case HMI_SCREEN_WEATHER:
            lines[0] = "Temperature: 23.8 C";
            lines[1] = "Humidity: 48%";
            lines[2] = "Wind: 8.4 km/h";
            lines[3] = "Rain: 0.0 mm";
            lines[4] = "ET: 2.1 mm";
            lines[5] = "Alerts: None";
            line_count = 6;
            break;
        case HMI_SCREEN_HYDRAULICS:
            lines[0] = "Hydraulic health: Excellent";
            lines[1] = "Current flow: 24.4 L/min";
            lines[2] = "Current pressure: 3.48 bar";
            lines[3] = "Leak status: Clear";
            lines[4] = "Restriction status: Clear";
            lines[5] = "Diagnostics: Ready";
            line_count = 6;
            break;
        case HMI_SCREEN_SETTINGS:
        default:
            lines[0] = "Controller name: Zmartify Irrigation";
            lines[1] = "Active user: Admin";
            lines[2] = "Wi-Fi status: Connected";
            lines[3] = "MQTT status: Connected";
            lines[4] = "Maintenance: Diagnostics, alarms, firmware";
            line_count = 5;
            break;
    }

    for (size_t i = 0; i < line_count; ++i)
    {
        lv_obj_t *line = lv_label_create(card);
        lv_label_set_text(line, lines[i]);
        lv_obj_set_style_text_color(line, lv_color_hex(0x2B2D42), 0);
        lv_obj_align(line, LV_ALIGN_TOP_LEFT, 18, 84 + (int)i * 36);
    }

    return card;
}

static void create_all_screen_bodies(lv_obj_t *parent)
{
    for (size_t i = 0; i < HMI_SCREEN_COUNT; ++i)
    {
        s_screen_cards[i] = create_screen_body(parent, (hmi_screen_t)i);
        if ((hmi_screen_t)i != s_active_screen)
        {
            lv_obj_add_flag(s_screen_cards[i], LV_OBJ_FLAG_HIDDEN);
        }
    }
}

static void update_status_labels(void)
{
    char uptime_buf[64];
    char system_buf[96];
    char zone_buf[64];
    char touch_buf[64];
    char perf_buf[64];

    if (!s_last_touch.valid)
    {
        snprintf(touch_buf, sizeof(touch_buf), "Touch: n/a");
    }
    else
    {
        snprintf(touch_buf,
                 sizeof(touch_buf),
                 "Touch: %s (%d,%d)",
                 s_last_touch.pressed ? "DOWN" : "UP",
                 (int)s_last_touch.x,
                 (int)s_last_touch.y);
    }

    int64_t now_us = esp_timer_get_time();
    if (s_last_perf_us == 0)
    {
        s_last_perf_us = now_us;
    }

    int64_t dt_us = now_us - s_last_perf_us;
    if (dt_us < 1000)
    {
        dt_us = 1000;
    }

    uint32_t flush_now = s_flush_count;
    uint32_t flush_delta = flush_now - s_last_flush_count;
    float fps = ((float)flush_delta * 1000000.0f) / (float)dt_us;
    float frame_ms = 1000.0f / (fps > 0.01f ? fps : 0.01f);

    uint32_t uptime_s = (uint32_t)(now_us / 1000000LL);
    uint32_t up_h = uptime_s / 3600U;
    uint32_t up_m = (uptime_s % 3600U) / 60U;
    uint32_t up_s = uptime_s % 60U;

    snprintf(uptime_buf,
             sizeof(uptime_buf),
             "Uptime: %02u:%02u:%02u",
             (unsigned int)up_h,
             (unsigned int)up_m,
             (unsigned int)up_s);
    snprintf(system_buf, sizeof(system_buf), "System: Online  |  Display: 1024x600 RGB  |  Touch: GT911");
    snprintf(zone_buf,
             sizeof(zone_buf),
             "Screen: %s  |  %s",
             s_screen_titles[s_active_screen],
             screen_summary(s_active_screen));
    if (flush_delta == 0)
    {
        snprintf(perf_buf, sizeof(perf_buf), "Render: idle (no flush)");
    }
    else
    {
        snprintf(perf_buf, sizeof(perf_buf), "Render: %.1f fps (%.1f ms)", fps, frame_ms);
    }

    set_label_text_if_changed(s_label_uptime, uptime_buf);
    set_label_text_if_changed(s_label_subtitle, s_screen_titles[s_active_screen]);
    set_label_text_if_changed(s_label_system, system_buf);
    set_label_text_if_changed(s_label_zone, zone_buf);
    set_label_text_if_changed(s_label_touch, touch_buf);
    set_label_text_if_changed(s_label_perf, perf_buf);
    set_label_text_if_changed(s_label_note, screen_summary(s_active_screen));

    s_last_perf_us = now_us;
    s_last_flush_count = flush_now;
    s_last_status_update_us = now_us;
}

static void create_home_screen(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xEEF3F8), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_clear_flag(scr, LV_OBJ_FLAG_SCROLL_CHAIN);
    lv_obj_set_scrollbar_mode(scr, LV_SCROLLBAR_MODE_OFF);

    lv_obj_t *header = lv_obj_create(scr);
    lv_obj_set_size(header, LCD_H_RES - 48, 86);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 16);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x1D3557), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 18, 0);
    lv_obj_set_style_shadow_width(header, 0, 0);
    lv_obj_set_style_pad_all(header, 0, 0);
    lv_obj_clear_flag(header, LV_OBJ_FLAG_SCROLLABLE | LV_OBJ_FLAG_SCROLL_ELASTIC | LV_OBJ_FLAG_SCROLL_MOMENTUM);
    lv_obj_set_style_anim_time(header, 0, LV_PART_MAIN | LV_STATE_ANY);

    s_label_title = lv_label_create(header);
    lv_label_set_text(s_label_title, "Zmartify Irrigation");
    lv_obj_set_style_text_color(s_label_title, lv_color_hex(0xF1FAEE), 0);
    lv_obj_align(s_label_title, LV_ALIGN_TOP_LEFT, 18, 12);

    s_label_subtitle = lv_label_create(header);
    lv_label_set_text(s_label_subtitle, s_screen_titles[s_active_screen]);
    lv_obj_set_style_text_color(s_label_subtitle, lv_color_hex(0xA8DADC), 0);
    lv_obj_align_to(s_label_subtitle, s_label_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 4);

    s_label_uptime = lv_label_create(header);
    lv_label_set_text(s_label_uptime, "Uptime: 00:00:00");
    lv_obj_set_style_text_color(s_label_uptime, lv_color_hex(0xF1FAEE), 0);
    lv_obj_align(s_label_uptime, LV_ALIGN_TOP_RIGHT, -18, 12);

    s_label_system = lv_label_create(header);
    lv_label_set_long_mode(s_label_system, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_label_system, 420);
    lv_label_set_text(s_label_system, "System: Online  |  Display: 1024x600 RGB  |  Touch: GT911");
    lv_obj_set_style_text_color(s_label_system, lv_color_hex(0xDDEBF7), 0);
    lv_obj_align(s_label_system, LV_ALIGN_BOTTOM_RIGHT, -18, -10);

    lv_obj_t *nav = lv_obj_create(scr);
    lv_obj_set_size(nav, HMI_NAV_W, HMI_NAV_H);
    lv_obj_align(nav, LV_ALIGN_TOP_MID, 0, HMI_NAV_Y);
    lv_obj_set_style_bg_color(nav, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(nav, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(nav, lv_color_hex(0xC7D6E3), 0);
    lv_obj_set_style_border_width(nav, 2, 0);
    lv_obj_set_style_radius(nav, 14, 0);
    lv_obj_set_style_shadow_width(nav, 0, 0);
    lv_obj_set_style_pad_all(nav, 0, 0);
    lv_obj_clear_flag(nav, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_clear_flag(nav, LV_OBJ_FLAG_SCROLL_ON_FOCUS);
    lv_obj_set_style_anim_time(nav, 0, LV_PART_MAIN | LV_STATE_ANY);

    int nav_button_w = (HMI_NAV_W - (HMI_NAV_GAP * 6)) / HMI_SCREEN_COUNT;
    int nav_button_x = HMI_NAV_GAP;
    for (size_t i = 0; i < HMI_SCREEN_COUNT; ++i)
    {
        lv_obj_t *button = lv_obj_create(nav);
        lv_obj_remove_style_all(button);
        lv_obj_set_size(button, nav_button_w, HMI_NAV_BUTTON_H);
        lv_obj_set_pos(button, nav_button_x, HMI_NAV_BUTTON_Y_IN_NAV);
        lv_obj_clear_flag(button,
                          LV_OBJ_FLAG_CLICKABLE |
                          LV_OBJ_FLAG_SCROLLABLE |
                          LV_OBJ_FLAG_SCROLL_ELASTIC |
                          LV_OBJ_FLAG_SCROLL_MOMENTUM |
                          LV_OBJ_FLAG_SCROLL_CHAIN |
                          LV_OBJ_FLAG_GESTURE_BUBBLE |
                          LV_OBJ_FLAG_SCROLL_ON_FOCUS);
        lv_obj_set_style_radius(button, 12, 0);
        lv_obj_set_style_pad_all(button, 0, 0);
        lv_obj_set_style_bg_opa(button, LV_OPA_COVER, 0);
        lv_obj_set_style_border_width(button, 1, 0);
        lv_obj_set_style_border_color(button, lv_color_hex(0xC7D6E3), 0);
        lv_obj_set_style_bg_color(button, lv_color_hex(0xE8EEF5), 0);
        lv_obj_set_style_shadow_width(button, 0, 0);
        lv_obj_set_style_translate_y(button, 0, LV_STATE_ANY);
        lv_obj_set_style_transform_height(button, 0, LV_STATE_ANY);
        lv_obj_set_style_transform_width(button, 0, LV_STATE_ANY);
        lv_obj_set_style_anim_time(button, 0, LV_PART_MAIN | LV_STATE_ANY);

        lv_obj_t *button_label = lv_label_create(button);
        lv_label_set_text(button_label, s_screen_titles[i]);
        lv_obj_set_style_text_color(button_label, lv_color_hex(0x1D3557), 0);
        lv_obj_center(button_label);

        s_nav_buttons[i] = button;
        nav_button_x += nav_button_w + HMI_NAV_GAP;
    }

    s_content_root = lv_obj_create(scr);
    lv_obj_set_size(s_content_root, LCD_H_RES - 48, 330);
    lv_obj_align(s_content_root, LV_ALIGN_TOP_MID, 0, 172);
    lv_obj_set_style_bg_color(s_content_root, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(s_content_root, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(s_content_root, lv_color_hex(0xC7D6E3), 0);
    lv_obj_set_style_border_width(s_content_root, 2, 0);
    lv_obj_set_style_radius(s_content_root, 18, 0);
    lv_obj_set_style_shadow_width(s_content_root, 0, 0);
    lv_obj_set_style_pad_all(s_content_root, 0, 0);
    lv_obj_clear_flag(s_content_root, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_anim_time(s_content_root, 0, LV_PART_MAIN | LV_STATE_ANY);

    lv_obj_t *footer = lv_obj_create(scr);
    lv_obj_set_size(footer, LCD_H_RES - 48, 72);
    lv_obj_align(footer, LV_ALIGN_TOP_MID, 0, 512);
    lv_obj_set_style_bg_color(footer, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_bg_opa(footer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_color(footer, lv_color_hex(0xC7D6E3), 0);
    lv_obj_set_style_border_width(footer, 2, 0);
    lv_obj_set_style_radius(footer, 14, 0);
    lv_obj_set_style_shadow_width(footer, 0, 0);
    lv_obj_set_style_pad_all(footer, 0, 0);
    lv_obj_clear_flag(footer, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_style_anim_time(footer, 0, LV_PART_MAIN | LV_STATE_ANY);

    s_label_zone = lv_label_create(footer);
    lv_obj_set_size(s_label_zone, LCD_H_RES - 92, 20);
    lv_obj_align(s_label_zone, LV_ALIGN_TOP_LEFT, 18, 8);
    lv_label_set_text(s_label_zone, "Screen: Dashboard  |  Operational overview and quick actions");
    lv_obj_set_style_text_color(s_label_zone, lv_color_hex(0x2B2D42), 0);

    s_label_touch = lv_label_create(footer);
    lv_obj_set_size(s_label_touch, 270, 18);
    lv_obj_align(s_label_touch, LV_ALIGN_TOP_LEFT, 18, 34);
    lv_label_set_text(s_label_touch, "Touch: n/a");
    lv_obj_set_style_text_color(s_label_touch, lv_color_hex(0x005F73), 0);

    s_label_perf = lv_label_create(footer);
    lv_obj_set_size(s_label_perf, 260, 18);
    lv_obj_align(s_label_perf, LV_ALIGN_TOP_LEFT, 310, 34);
    lv_label_set_text(s_label_perf, "Render: collecting...");
    lv_obj_set_style_text_color(s_label_perf, lv_color_hex(0x9B2226), 0);

    s_label_note = lv_label_create(footer);
    lv_obj_set_size(s_label_note, LCD_H_RES - 92 - 592, 18);
    lv_obj_align(s_label_note, LV_ALIGN_TOP_LEFT, 592, 34);
    lv_label_set_text(s_label_note, screen_summary(s_active_screen));
    lv_obj_set_style_text_color(s_label_note, lv_color_hex(0x4A4E69), 0);

    s_active_screen = HMI_SCREEN_DASHBOARD;
    create_all_screen_bodies(s_content_root);
    update_nav_highlight();

    s_last_perf_us = esp_timer_get_time();
}

#if LVGL_VERSION_MAJOR >= 9
static void lvgl_flush_cb(lv_display_t *display, const lv_area_t *area, uint8_t *color_map)
{
    (void)display;
    esp_err_t err = esp_lcd_panel_draw_bitmap(
        s_panel,
        area->x1,
        area->y1,
        area->x2 + 1,
        area->y2 + 1,
        color_map);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "draw_bitmap failed: %d", err);
    }
    s_flush_count++;
    lv_display_flush_ready(display);
}

static void lvgl_touch_read_cb(lv_indev_t *indev, lv_indev_data_t *data)
{
    (void)indev;
    uint16_t x = 0;
    uint16_t y = 0;
    bool pressed = false;
    bool read_ok = hmi_touch_read(&x, &y, &pressed);

    if (read_ok)
    {
        s_last_touch.valid = true;
        s_last_touch.pressed = pressed;
        if (pressed)
        {
            s_last_touch.x = (int16_t)x;
            s_last_touch.y = (int16_t)y;
            clamp_touch_to_panel(&s_last_touch.x, &s_last_touch.y);
        }
    }
    else
    {
        /* On transient I2C read errors, force release so LVGL can still complete click sequences. */
        s_last_touch.valid = true;
        s_last_touch.pressed = false;
    }

    if (s_touch_was_pressed && read_ok && !pressed)
    {
        process_nav_tap(s_last_touch.x, s_last_touch.y);
    }
    s_touch_was_pressed = s_last_touch.pressed;

    data->state = (s_last_touch.valid && s_last_touch.pressed) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    data->point.x = s_last_touch.x;
    data->point.y = s_last_touch.y;
}
#else
static void lvgl_flush_cb(lv_disp_drv_t *disp_drv, const lv_area_t *area, lv_color_t *color_map)
{
    esp_err_t err = esp_lcd_panel_draw_bitmap(
        s_panel,
        area->x1,
        area->y1,
        area->x2 + 1,
        area->y2 + 1,
        color_map);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "draw_bitmap failed: %d", err);
    }
    s_flush_count++;
    lv_disp_flush_ready(disp_drv);
}

static void lvgl_touch_read_cb(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    (void)indev_drv;
    uint16_t x = 0;
    uint16_t y = 0;
    bool pressed = false;
    bool read_ok = hmi_touch_read(&x, &y, &pressed);

    if (read_ok)
    {
        s_last_touch.valid = true;
        s_last_touch.pressed = pressed;
        if (pressed)
        {
            s_last_touch.x = (int16_t)x;
            s_last_touch.y = (int16_t)y;
            clamp_touch_to_panel(&s_last_touch.x, &s_last_touch.y);
        }
    }
    else
    {
        /* On transient I2C read errors, force release so LVGL can still complete click sequences. */
        s_last_touch.valid = true;
        s_last_touch.pressed = false;
    }

    if (s_touch_was_pressed && read_ok && !pressed)
    {
        process_nav_tap(s_last_touch.x, s_last_touch.y);
    }
    s_touch_was_pressed = s_last_touch.pressed;

    data->state = (s_last_touch.valid && s_last_touch.pressed) ? LV_INDEV_STATE_PRESSED : LV_INDEV_STATE_RELEASED;
    data->point.x = s_last_touch.x;
    data->point.y = s_last_touch.y;
}
#endif

static bool init_rgb_panel(void)
{
    return hmi_7b_rgb_init(&s_panel);
}

#if HMI_RGB_BYPASS_DIAG_MODE
static bool rgb_diag_apply_profile(size_t profile_idx)
{
    if (profile_idx >= (sizeof(s_rgb_profiles) / sizeof(s_rgb_profiles[0])))
    {
        return false;
    }

    if (s_panel != NULL)
    {
        (void)hmi_7b_rgb_deinit(s_panel);
        s_panel = NULL;
        vTaskDelay(pdMS_TO_TICKS(40));
    }

    if (!hmi_7b_rgb_init_with_flags(&s_panel, &s_rgb_profiles[profile_idx].flags))
    {
        ESP_LOGE(TAG, "RGB profile init failed: %s", s_rgb_profiles[profile_idx].name);
        return false;
    }

    ESP_LOGW(TAG,
             "RGB PROFILE %u/%u: %s",
             (unsigned)(profile_idx + 1),
             (unsigned)(sizeof(s_rgb_profiles) / sizeof(s_rgb_profiles[0])),
             s_rgb_profiles[profile_idx].name);
    return true;
}

static void rgb_diag_fill_screen(uint16_t color)
{
    if (s_rgb_diag_frame == NULL)
    {
        return;
    }

    size_t px_count = (size_t)LCD_H_RES * (size_t)LCD_V_RES;
    for (size_t i = 0; i < px_count; ++i)
    {
        s_rgb_diag_frame[i] = color;
    }

    esp_err_t err = esp_lcd_rgb_panel_restart(s_panel);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "rgb restart failed: %d", err);
    }

    err = esp_lcd_panel_disp_on_off(s_panel, true);
    if (err != ESP_OK && err != ESP_ERR_NOT_SUPPORTED)
    {
        ESP_LOGW(TAG, "rgb disp_on_off failed: %d", err);
    }

    err = esp_lcd_panel_draw_bitmap(s_panel, 0, 0, LCD_H_RES, LCD_V_RES, s_rgb_diag_frame);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "rgb diag draw failed: %d", err);
    }
}

static void rgb_diag_task(void *arg)
{
    (void)arg;

    static const uint16_t colors[] = {
        0xF800, /* red */
        0x07E0, /* green */
        0x001F, /* blue */
        0xFFFF, /* white */
        0x0000, /* black */
    };

    size_t idx = 0;
#if HMI_RGB_PROFILE_SWEEP_MODE
    size_t profile_idx = 0;
    uint32_t frame_count = 0;
#endif
    while (true)
    {
#if HMI_RGB_PROFILE_SWEEP_MODE
        if ((frame_count % 6U) == 0U)
        {
            if (!rgb_diag_apply_profile(profile_idx))
            {
                ESP_LOGW(TAG, "RGB profile apply failed, retrying current profile");
            }
            else
            {
                profile_idx = (profile_idx + 1U) % (sizeof(s_rgb_profiles) / sizeof(s_rgb_profiles[0]));
            }
        }
#endif

        rgb_diag_fill_screen(colors[idx]);
        idx = (idx + 1U) % (sizeof(colors) / sizeof(colors[0]));
#if HMI_RGB_PROFILE_SWEEP_MODE
        frame_count++;
#endif
        vTaskDelay(pdMS_TO_TICKS(400));
    }
}

static bool start_rgb_diag_mode(void)
{
    s_rgb_diag_frame = heap_caps_malloc(
        (size_t)LCD_H_RES * (size_t)LCD_V_RES * sizeof(uint16_t),
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_rgb_diag_frame == NULL)
    {
        ESP_LOGE(TAG, "RGB diag frame alloc failed");
        return false;
    }

    if (!rgb_diag_apply_profile(0))
    {
        return false;
    }

    BaseType_t task_ok = xTaskCreatePinnedToCore(
        rgb_diag_task,
        "hmi_rgb_diag",
        4096,
        NULL,
        4,
        &s_rgb_diag_task,
        tskNO_AFFINITY);
    if (task_ok != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create RGB diag task");
        return false;
    }

    ESP_LOGW(TAG, "RGB BYPASS DIAG MODE ACTIVE (LVGL disabled, stable profile)");
    return true;
}
#endif

static bool init_lvgl_core(void)
{
    lv_init();

    size_t draw_pixels = LCD_H_RES * LVGL_DRAW_LINES;
    size_t draw_bytes = draw_pixels * sizeof(lv_color_t);

    s_draw_buf_1 = heap_caps_malloc(draw_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    s_draw_buf_2 = heap_caps_malloc(draw_bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
    if (s_draw_buf_1 == NULL || s_draw_buf_2 == NULL)
    {
        ESP_LOGE(TAG, "LVGL draw buffers alloc failed (%u bytes each)", (unsigned)draw_bytes);
        return false;
    }

#if LVGL_VERSION_MAJOR >= 9
    lv_display_t *display = lv_display_create(LCD_H_RES, LCD_V_RES);
    if (display == NULL)
    {
        ESP_LOGE(TAG, "lv_display_create failed");
        return false;
    }

    lv_display_set_buffers(display, s_draw_buf_1, s_draw_buf_2, draw_bytes, LV_DISPLAY_RENDER_MODE_PARTIAL);
    lv_display_set_flush_cb(display, lvgl_flush_cb);

    lv_indev_t *touch = lv_indev_create();
    if (touch == NULL)
    {
        ESP_LOGE(TAG, "lv_indev_create failed");
        return false;
    }
    lv_indev_set_type(touch, LV_INDEV_TYPE_POINTER);
    lv_indev_set_read_cb(touch, lvgl_touch_read_cb);
#else
    static lv_disp_draw_buf_t draw_buf;
    lv_disp_draw_buf_init(&draw_buf, s_draw_buf_1, s_draw_buf_2, draw_pixels);

    static lv_disp_drv_t disp_drv;
    lv_disp_drv_init(&disp_drv);
    disp_drv.hor_res = LCD_H_RES;
    disp_drv.ver_res = LCD_V_RES;
    disp_drv.flush_cb = lvgl_flush_cb;
    disp_drv.draw_buf = &draw_buf;
    (void)lv_disp_drv_register(&disp_drv);

    static lv_indev_drv_t indev_drv;
    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = lvgl_touch_read_cb;
    (void)lv_indev_drv_register(&indev_drv);
#endif

    esp_timer_create_args_t timer_args = {
        .callback = lvgl_tick_cb,
        .name = "lvgl_tick",
    };
    esp_err_t err = esp_timer_create(&timer_args, &s_tick_timer);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_timer_create failed: %d", err);
        return false;
    }

    err = esp_timer_start_periodic(s_tick_timer, LVGL_TICK_MS * 1000);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_timer_start_periodic failed: %d", err);
        return false;
    }

    create_home_screen();
    update_status_labels();
    ESP_LOGI(TAG, "LVGL initialized (%d.%d)", LVGL_VERSION_MAJOR, LVGL_VERSION_MINOR);
    return true;
}

static void lvgl_task(void *arg)
{
    (void)arg;
    while (true)
    {
        uint32_t wait_ms = LVGL_TASK_MAX_DELAY_MS;
        if (lvgl_lock(pdMS_TO_TICKS(50)))
        {
#if LVGL_VERSION_MAJOR >= 9
            wait_ms = lv_timer_handler();
#else
            wait_ms = lv_timer_handler();
#endif
            apply_pending_screen_change();
            int64_t now_us = esp_timer_get_time();
            if (s_last_status_update_us == 0 ||
                (now_us - s_last_status_update_us) >= (STATUS_UPDATE_PERIOD_MS * 1000))
            {
                update_status_labels();
            }
            lvgl_unlock();
        }

        if (wait_ms < LVGL_TASK_MIN_DELAY_MS)
        {
            wait_ms = LVGL_TASK_MIN_DELAY_MS;
        }
        if (wait_ms > LVGL_TASK_MAX_DELAY_MS)
        {
            wait_ms = LVGL_TASK_MAX_DELAY_MS;
        }

        vTaskDelay(pdMS_TO_TICKS(wait_ms));
    }
}

bool hmi_lvgl_port_init(void)
{
    if (s_panel != NULL)
    {
        return true;
    }

    s_lvgl_mutex = xSemaphoreCreateRecursiveMutex();
    if (s_lvgl_mutex == NULL)
    {
        ESP_LOGE(TAG, "LVGL mutex create failed");
        return false;
    }

#if HMI_RGB_BYPASS_DIAG_MODE
    return start_rgb_diag_mode();
#endif

    if (!init_rgb_panel())
    {
        return false;
    }

    if (!lvgl_lock(pdMS_TO_TICKS(100)))
    {
        ESP_LOGE(TAG, "LVGL lock timed out during init");
        return false;
    }

    bool lv_ok = init_lvgl_core();
    lvgl_unlock();
    if (!lv_ok)
    {
        return false;
    }

    if (!hmi_touch_init())
    {
        ESP_LOGW(TAG, "Touch init failed; continuing with display-only mode");
    }

    BaseType_t task_ok = xTaskCreatePinnedToCore(
        lvgl_task,
        "hmi_lvgl",
        8192,
        NULL,
        4,
        &s_lvgl_task,
        tskNO_AFFINITY);
    if (task_ok != pdPASS)
    {
        ESP_LOGE(TAG, "Failed to create LVGL task");
        return false;
    }

    ESP_LOGI(TAG, "LVGL display+touch bridge ready");
    return true;
}
