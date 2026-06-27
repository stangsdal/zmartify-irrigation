#include "hmi_7b_ioexp.h"

#include "hmi_7b_bus.h"

#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "hmi_7b_ioexp";

#define HMI_7B_IOEXP_REG_MODE    0x02
#define HMI_7B_IOEXP_REG_OUTPUT  0x03
#define HMI_7B_IOEXP_REG_PWM     0x05
#define HMI_7B_IOEXP_OUTPUT_HIGH 0xFF

#define HMI_7B_IO_TOUCH_RST_BIT  1
#define HMI_7B_IO_BACKLIGHT_BIT  2
#define HMI_7B_IO_LCD_RST_BIT    3
#define HMI_7B_IO_USB_CAN_BIT    5

/* Some 7B carrier revisions gate BL EN on IO2 with inverted polarity. */
#define HMI_7B_BACKLIGHT_ACTIVE_LOW 0

static bool s_ready = false;
static uint8_t s_output = HMI_7B_IOEXP_OUTPUT_HIGH;
static uint8_t s_addr = HMI_7B_IOEXP_ADDR;

static bool ioexp_select_address(void)
{
    if (hmi_7b_bus_probe(HMI_7B_IOEXP_ADDR))
    {
        s_addr = HMI_7B_IOEXP_ADDR;
        return true;
    }

    for (uint8_t addr = 0x20; addr <= 0x27; ++addr)
    {
        if (addr == HMI_7B_IOEXP_ADDR)
        {
            continue;
        }
        if (hmi_7b_bus_probe(addr))
        {
            s_addr = addr;
            ESP_LOGW(TAG, "IO expander fallback address selected: 0x%02X", s_addr);
            return true;
        }
    }

    return false;
}

static bool ioexp_write_output(void)
{
    return hmi_7b_bus_write_reg8(s_addr, HMI_7B_IOEXP_REG_OUTPUT, &s_output, 1);
}

bool hmi_7b_ioexp_set_backlight_pwm(uint8_t value)
{
    if (!s_ready && !hmi_7b_ioexp_init())
    {
        return false;
    }

    return hmi_7b_bus_write_reg8(s_addr, HMI_7B_IOEXP_REG_PWM, &value, 1);
}

bool hmi_7b_ioexp_init(void)
{
    if (s_ready)
    {
        return true;
    }

    if (!hmi_7b_bus_init())
    {
        ESP_LOGE(TAG, "I2C init failed");
        return false;
    }

    if (!ioexp_select_address())
    {
        ESP_LOGW(TAG, "IO expander not detected on 0x20-0x27");
        return false;
    }

    uint8_t mode = HMI_7B_IOEXP_OUTPUT_HIGH;
    if (!hmi_7b_bus_write_reg8(s_addr, HMI_7B_IOEXP_REG_MODE, &mode, 1))
    {
        ESP_LOGW(TAG, "Failed to set IO expander mode register");
        return false;
    }

    s_output = HMI_7B_IOEXP_OUTPUT_HIGH;
    /* Keep board in USB mode (IO5=0) per Waveshare IO assignment. */
    s_output &= (uint8_t)~(1U << HMI_7B_IO_USB_CAN_BIT);
    if (!ioexp_write_output())
    {
        ESP_LOGW(TAG, "Failed to set IO expander output defaults");
        return false;
    }

    /* Waveshare IO extension PWM: lower values produce brighter backlight. */
    uint8_t pwm = 0; /* full brightness */
    if (!hmi_7b_bus_write_reg8(s_addr, HMI_7B_IOEXP_REG_PWM, &pwm, 1))
    {
        ESP_LOGW(TAG, "Failed to set IO expander PWM register");
    }

    s_ready = true;
    return true;
}

bool hmi_7b_ioexp_set_output_bit(uint8_t bit, bool high)
{
    if (!s_ready && !hmi_7b_ioexp_init())
    {
        return false;
    }

    if (bit > 7)
    {
        return false;
    }

    if (high)
    {
        s_output |= (uint8_t)(1U << bit);
    }
    else
    {
        s_output &= (uint8_t)~(1U << bit);
    }

    return ioexp_write_output();
}

bool hmi_7b_ioexp_set_backlight(bool on)
{
    /* Keep PWM in bright range and gate with IO2 enable line. */
    (void)hmi_7b_ioexp_set_backlight_pwm(0);
#if HMI_7B_BACKLIGHT_ACTIVE_LOW
    return hmi_7b_ioexp_set_output_bit(HMI_7B_IO_BACKLIGHT_BIT, !on);
#else
    return hmi_7b_ioexp_set_output_bit(HMI_7B_IO_BACKLIGHT_BIT, on);
#endif
}

bool hmi_7b_ioexp_lcd_reset_pulse(void)
{
    if (!hmi_7b_ioexp_set_output_bit(HMI_7B_IO_LCD_RST_BIT, false))
    {
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(20));

    if (!hmi_7b_ioexp_set_output_bit(HMI_7B_IO_LCD_RST_BIT, true))
    {
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(120));

    return true;
}

bool hmi_7b_ioexp_touch_reset_pulse(void)
{
    if (!hmi_7b_ioexp_set_output_bit(HMI_7B_IO_TOUCH_RST_BIT, false))
    {
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(20));

    if (!hmi_7b_ioexp_set_output_bit(HMI_7B_IO_TOUCH_RST_BIT, true))
    {
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(120));

    return true;
}
