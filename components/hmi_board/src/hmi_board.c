#include "hmi_board.h"
#include "hmi_7b_ioexp.h"
#include "hmi_7b_touch.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "hmi_board";

#define TOUCH_GT911_ADDR_PRIMARY  0x5D
#define TOUCH_GT911_ADDR_ALT      0x14

extern bool hmi_touch_detect(void);
extern bool hmi_lvgl_port_init(void);

static hmi_board_status_t s_status;

static void run_backlight_probe_sequence(void)
{
    if (!hmi_7b_ioexp_init())
    {
        ESP_LOGW(TAG, "Skipping BL probe: IO expander init failed");
        return;
    }

    ESP_LOGW(TAG, "BL probe: toggling IO2 low/high and PWM 0/247 (3 cycles)");
    for (int i = 0; i < 3; ++i)
    {
        (void)hmi_7b_ioexp_set_backlight_pwm(0);
        (void)hmi_7b_ioexp_set_output_bit(2, false);
        vTaskDelay(pdMS_TO_TICKS(300));

        (void)hmi_7b_ioexp_set_backlight_pwm(247);
        (void)hmi_7b_ioexp_set_output_bit(2, true);
        vTaskDelay(pdMS_TO_TICKS(300));
    }
}

static bool enable_backlight(void)
{
    if (!hmi_7b_ioexp_init())
    {
        ESP_LOGW(TAG, "IO expander init failed; backlight unavailable");
        return false;
    }

    return hmi_7b_ioexp_set_backlight(true);
}

bool hmi_board_init(void)
{
    bool io_ok = hmi_7b_ioexp_init();
    if (!io_ok)
    {
        ESP_LOGW(TAG, "7B IO expander not detected");
    }

    run_backlight_probe_sequence();

    /* Strict replacement order from 7B docs: IO expander -> backlight on -> LCD reset. */
    s_status.backlight_enabled = enable_backlight();
    if (!s_status.backlight_enabled)
    {
        ESP_LOGW(TAG, "Initial backlight enable failed");
    }

    if (!hmi_7b_ioexp_lcd_reset_pulse())
    {
        ESP_LOGW(TAG, "LCD reset pulse failed");
    }

    if (!hmi_7b_ioexp_touch_reset_pulse())
    {
        ESP_LOGW(TAG, "Touch reset pulse failed");
    }

    s_status.panel_ready = hmi_lvgl_port_init();
    if (!s_status.panel_ready)
    {
        ESP_LOGW(TAG, "Display/LVGL init failed");
    }

    /* Re-assert BL after panel init as Waveshare examples do via helper call sites. */
    s_status.backlight_enabled = enable_backlight() || s_status.backlight_enabled;

    s_status.touch_present = hmi_7b_touch_detect();
    if (s_status.touch_present)
    {
        ESP_LOGI(TAG, "Touch controller detected at 0x%02X/0x%02X",
                 TOUCH_GT911_ADDR_PRIMARY,
                 TOUCH_GT911_ADDR_ALT);
    }
    else
    {
        ESP_LOGW(TAG, "Touch controller not detected");
    }

    ESP_LOGI(TAG, "Bring-up status: backlight=%d touch=%d panel=%d",
             s_status.backlight_enabled,
             s_status.touch_present,
             s_status.panel_ready);

    return s_status.backlight_enabled || s_status.panel_ready || s_status.touch_present || io_ok;
}

void hmi_board_get_status(hmi_board_status_t *out_status)
{
    if (out_status == NULL)
    {
        return;
    }
    *out_status = s_status;
}
