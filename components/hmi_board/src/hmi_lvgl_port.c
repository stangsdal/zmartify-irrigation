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
#include <stdio.h>

static const char *TAG = "hmi_lvgl";

#define LCD_H_RES                      HMI_7B_LCD_H_RES
#define LCD_V_RES                      HMI_7B_LCD_V_RES

#define LVGL_TICK_MS                   2
#define LVGL_TASK_MIN_DELAY_MS         2
#define LVGL_TASK_MAX_DELAY_MS         16
#define LVGL_DRAW_LINES                40

/* Temporary strict RGB-path diagnostic: bypass LVGL and draw full-screen color bars. */
#define HMI_RGB_BYPASS_DIAG_MODE       0
#define HMI_RGB_PROFILE_SWEEP_MODE     0

typedef struct
{
    int16_t x;
    int16_t y;
    bool pressed;
    bool valid;
} hmi_touch_state_t;

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

static volatile uint32_t s_flush_count = 0;
static uint32_t s_last_flush_count = 0;
static int64_t s_last_perf_us = 0;
static hmi_touch_state_t s_last_touch = {0};

static lv_color_t *s_draw_buf_1 = NULL;
static lv_color_t *s_draw_buf_2 = NULL;

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
                 "Touch: %s (%d, %d)",
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
    snprintf(zone_buf, sizeof(zone_buf), "Irrigation: Idle (no active zone)");
    snprintf(perf_buf, sizeof(perf_buf), "Render: %.1f fps (%.1f ms)", fps, frame_ms);

    lv_label_set_text(s_label_uptime, uptime_buf);
    lv_label_set_text(s_label_system, system_buf);
    lv_label_set_text(s_label_zone, zone_buf);
    lv_label_set_text(s_label_touch, touch_buf);
    lv_label_set_text(s_label_perf, perf_buf);

    s_last_perf_us = now_us;
    s_last_flush_count = flush_now;
}

static void create_home_screen(void)
{
    lv_obj_t *scr = lv_scr_act();
    lv_obj_set_style_bg_color(scr, lv_color_hex(0xEEF3F8), 0);
    lv_obj_set_style_bg_opa(scr, LV_OPA_COVER, 0);

    lv_obj_t *header = lv_obj_create(scr);
    lv_obj_set_size(header, LCD_H_RES - 48, 92);
    lv_obj_align(header, LV_ALIGN_TOP_MID, 0, 20);
    lv_obj_set_style_bg_color(header, lv_color_hex(0x1D3557), 0);
    lv_obj_set_style_border_width(header, 0, 0);
    lv_obj_set_style_radius(header, 18, 0);
    lv_obj_set_style_shadow_width(header, 0, 0);

    s_label_title = lv_label_create(header);
    lv_label_set_text(s_label_title, "Zmartify Irrigation");
    lv_obj_set_style_text_color(s_label_title, lv_color_hex(0xF1FAEE), 0);
    lv_obj_align(s_label_title, LV_ALIGN_TOP_LEFT, 20, 12);

    s_label_subtitle = lv_label_create(header);
    lv_label_set_text(s_label_subtitle, "Controller Home");
    lv_obj_set_style_text_color(s_label_subtitle, lv_color_hex(0xA8DADC), 0);
    lv_obj_align_to(s_label_subtitle, s_label_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 6);

    s_label_uptime = lv_label_create(header);
    lv_label_set_text(s_label_uptime, "Uptime: 00:00:00");
    lv_obj_set_style_text_color(s_label_uptime, lv_color_hex(0xF1FAEE), 0);
    lv_obj_align(s_label_uptime, LV_ALIGN_TOP_RIGHT, -20, 24);

    lv_obj_t *card_system = lv_obj_create(scr);
    lv_obj_set_size(card_system, LCD_H_RES - 48, 128);
    lv_obj_align(card_system, LV_ALIGN_TOP_MID, 0, 128);
    lv_obj_set_style_bg_color(card_system, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_color(card_system, lv_color_hex(0xC7D6E3), 0);
    lv_obj_set_style_border_width(card_system, 2, 0);
    lv_obj_set_style_radius(card_system, 16, 0);
    lv_obj_set_style_shadow_width(card_system, 0, 0);

    s_label_system = lv_label_create(card_system);
    lv_label_set_long_mode(s_label_system, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_label_system, LCD_H_RES - 96);
    lv_label_set_text(s_label_system, "System: Online");
    lv_obj_set_style_text_color(s_label_system, lv_color_hex(0x2B2D42), 0);
    lv_obj_align(s_label_system, LV_ALIGN_TOP_LEFT, 18, 16);

    s_label_zone = lv_label_create(card_system);
    lv_label_set_text(s_label_zone, "Irrigation: Idle");
    lv_obj_set_style_text_color(s_label_zone, lv_color_hex(0x2B2D42), 0);
    lv_obj_align_to(s_label_zone, s_label_system, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 14);

    lv_obj_t *card_runtime = lv_obj_create(scr);
    lv_obj_set_size(card_runtime, LCD_H_RES - 48, 206);
    lv_obj_align(card_runtime, LV_ALIGN_TOP_MID, 0, 274);
    lv_obj_set_style_bg_color(card_runtime, lv_color_hex(0xFFFFFF), 0);
    lv_obj_set_style_border_color(card_runtime, lv_color_hex(0xC7D6E3), 0);
    lv_obj_set_style_border_width(card_runtime, 2, 0);
    lv_obj_set_style_radius(card_runtime, 16, 0);
    lv_obj_set_style_shadow_width(card_runtime, 0, 0);

    lv_obj_t *runtime_title = lv_label_create(card_runtime);
    lv_label_set_text(runtime_title, "Runtime");
    lv_obj_set_style_text_color(runtime_title, lv_color_hex(0x1D3557), 0);
    lv_obj_align(runtime_title, LV_ALIGN_TOP_LEFT, 18, 12);

    s_label_touch = lv_label_create(card_runtime);
    lv_label_set_text(s_label_touch, "Touch: waiting...");
    lv_obj_set_style_text_color(s_label_touch, lv_color_hex(0x005F73), 0);
    lv_obj_align_to(s_label_touch, runtime_title, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 18);

    s_label_perf = lv_label_create(card_runtime);
    lv_label_set_text(s_label_perf, "Render: collecting...");
    lv_obj_set_style_text_color(s_label_perf, lv_color_hex(0x9B2226), 0);
    lv_obj_align_to(s_label_perf, s_label_touch, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 16);

    s_label_note = lv_label_create(scr);
    lv_label_set_long_mode(s_label_note, LV_LABEL_LONG_WRAP);
    lv_obj_set_width(s_label_note, LCD_H_RES - 80);
    lv_label_set_text(
        s_label_note,
        "Home screen active. Touch input and render timing are shown for commissioning while production irrigation widgets are integrated.");
    lv_obj_set_style_text_color(s_label_note, lv_color_hex(0x4A4E69), 0);
    lv_obj_align(s_label_note, LV_ALIGN_BOTTOM_MID, 0, -22);

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

    if (hmi_touch_read(&x, &y, &pressed))
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

    if (hmi_touch_read(&x, &y, &pressed))
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
            update_status_labels();
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
        ESP_LOGW(TAG, "Touch init failed; continuing with display-only diagnostics");
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
