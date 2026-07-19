#include "hmi_7b_rgb.h"

#include "esp_lcd_panel_rgb.h"
#include "esp_log.h"

static const char *TAG = "hmi_7b_rgb";

#if CONFIG_HMI_DISPLAY_PROFILE_HIGH_THROUGHPUT
#define LCD_PIXEL_CLOCK_HZ 30850000
#define LCD_PROFILE_NAME   "high-throughput"
#else
#define LCD_PIXEL_CLOCK_HZ 14000000
#define LCD_PROFILE_NAME   "low-risk"
#endif
#define LCD_RGB_DATA_WIDTH 16
#define LCD_RGB_FB_NUM     2

#define LCD_HSYNC_GPIO     46
#define LCD_VSYNC_GPIO     3
#define LCD_DE_GPIO        5
#define LCD_PCLK_GPIO      7
#define LCD_DISP_GPIO      (-1)

#define LCD_DATA0_GPIO     14
#define LCD_DATA1_GPIO     38
#define LCD_DATA2_GPIO     18
#define LCD_DATA3_GPIO     17
#define LCD_DATA4_GPIO     10
#define LCD_DATA5_GPIO     39
#define LCD_DATA6_GPIO     0
#define LCD_DATA7_GPIO     45
#define LCD_DATA8_GPIO     48
#define LCD_DATA9_GPIO     47
#define LCD_DATA10_GPIO    21
#define LCD_DATA11_GPIO    1
#define LCD_DATA12_GPIO    2
#define LCD_DATA13_GPIO    42
#define LCD_DATA14_GPIO    41
#define LCD_DATA15_GPIO    40

static const hmi_7b_rgb_flags_t k_default_flags = {
    .hsync_idle_low = true,
    .vsync_idle_low = true,
    .de_idle_high = false,
    .pclk_active_neg = true,
    .pclk_idle_high = false,
};

bool hmi_7b_rgb_init(esp_lcd_panel_handle_t *out_panel)
{
    return hmi_7b_rgb_init_with_flags(out_panel, &k_default_flags);
}

bool hmi_7b_rgb_deinit(esp_lcd_panel_handle_t panel)
{
    if (panel == NULL)
    {
        return true;
    }

    esp_err_t err = esp_lcd_panel_del(panel);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "esp_lcd_panel_del failed: %d", err);
        return false;
    }
    return true;
}

bool hmi_7b_rgb_register_event_callbacks(esp_lcd_panel_handle_t panel,
                                         const esp_lcd_rgb_panel_event_callbacks_t *callbacks,
                                         void *user_ctx)
{
    if (panel == NULL || callbacks == NULL)
    {
        return false;
    }

    esp_err_t err = esp_lcd_rgb_panel_register_event_callbacks(panel, callbacks, user_ctx);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "rgb event callback registration failed: %d", err);
        return false;
    }
    return true;
}

bool hmi_7b_rgb_init_with_flags(esp_lcd_panel_handle_t *out_panel, const hmi_7b_rgb_flags_t *flags)
{
    if (out_panel == NULL)
    {
        return false;
    }

    hmi_7b_rgb_flags_t cfg_flags = k_default_flags;
    if (flags != NULL)
    {
        cfg_flags = *flags;
    }

    esp_lcd_panel_handle_t panel = NULL;

    esp_lcd_rgb_panel_config_t panel_cfg = {
        .clk_src = LCD_CLK_SRC_DEFAULT,
        .timings = {
            .pclk_hz = LCD_PIXEL_CLOCK_HZ,
            .h_res = HMI_7B_LCD_H_RES,
            .v_res = HMI_7B_LCD_V_RES,
            .hsync_pulse_width = 162,
            .hsync_back_porch = 152,
            .hsync_front_porch = 48,
            .vsync_pulse_width = 45,
            .vsync_back_porch = 13,
            .vsync_front_porch = 3,
            .flags.hsync_idle_low = cfg_flags.hsync_idle_low ? 1U : 0U,
            .flags.vsync_idle_low = cfg_flags.vsync_idle_low ? 1U : 0U,
            .flags.de_idle_high = cfg_flags.de_idle_high ? 1U : 0U,
            .flags.pclk_active_neg = cfg_flags.pclk_active_neg ? 1U : 0U,
            .flags.pclk_idle_high = cfg_flags.pclk_idle_high ? 1U : 0U,
        },
        .data_width = LCD_RGB_DATA_WIDTH,
        .in_color_format = LCD_COLOR_FMT_RGB565,
        .out_color_format = LCD_COLOR_FMT_RGB565,
        .num_fbs = LCD_RGB_FB_NUM,
        .bounce_buffer_size_px = HMI_7B_LCD_H_RES * 10,
        .dma_burst_size = 64,
        .hsync_gpio_num = LCD_HSYNC_GPIO,
        .vsync_gpio_num = LCD_VSYNC_GPIO,
        .de_gpio_num = LCD_DE_GPIO,
        .pclk_gpio_num = LCD_PCLK_GPIO,
        .disp_gpio_num = LCD_DISP_GPIO,
        .data_gpio_nums = {
            LCD_DATA0_GPIO,
            LCD_DATA1_GPIO,
            LCD_DATA2_GPIO,
            LCD_DATA3_GPIO,
            LCD_DATA4_GPIO,
            LCD_DATA5_GPIO,
            LCD_DATA6_GPIO,
            LCD_DATA7_GPIO,
            LCD_DATA8_GPIO,
            LCD_DATA9_GPIO,
            LCD_DATA10_GPIO,
            LCD_DATA11_GPIO,
            LCD_DATA12_GPIO,
            LCD_DATA13_GPIO,
            LCD_DATA14_GPIO,
            LCD_DATA15_GPIO,
        },
        .flags.fb_in_psram = true,
    };

    esp_err_t err = esp_lcd_new_rgb_panel(&panel_cfg, &panel);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_lcd_new_rgb_panel failed: %d", err);
        return false;
    }

    err = esp_lcd_panel_init(panel);
    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "esp_lcd_panel_init failed: %d", err);
        (void)esp_lcd_panel_del(panel);
        return false;
    }

    /* Explicitly request panel wake/on and restart RGB DMA path. */
    err = esp_lcd_panel_disp_sleep(panel, false);
    if (err != ESP_OK && err != ESP_ERR_NOT_SUPPORTED)
    {
        ESP_LOGW(TAG, "esp_lcd_panel_disp_sleep(false) failed: %d", err);
    }

    err = esp_lcd_panel_disp_on_off(panel, true);
    if (err != ESP_OK && err != ESP_ERR_NOT_SUPPORTED)
    {
        ESP_LOGW(TAG, "esp_lcd_panel_disp_on_off(true) failed: %d", err);
    }

    err = esp_lcd_rgb_panel_restart(panel);
    if (err != ESP_OK)
    {
        ESP_LOGW(TAG, "esp_lcd_rgb_panel_restart failed: %d", err);
    }

    *out_panel = panel;
    ESP_LOGI(TAG,
             "RGB panel ready (%dx%d, pclk=%d, profile=%s) flags: hs=%d vs=%d de=%d pneg=%d pidh=%d",
             HMI_7B_LCD_H_RES,
             HMI_7B_LCD_V_RES,
             LCD_PIXEL_CLOCK_HZ,
             LCD_PROFILE_NAME,
             cfg_flags.hsync_idle_low,
             cfg_flags.vsync_idle_low,
             cfg_flags.de_idle_high,
             cfg_flags.pclk_active_neg,
             cfg_flags.pclk_idle_high);
    return true;
}
