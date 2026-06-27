#include "hmi_7b_touch.h"

#include "hmi_7b_bus.h"
#include "hmi_7b_ioexp.h"

#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "hmi_7b_touch";

#define GT911_ADDR_PRIMARY 0x5D
#define GT911_ADDR_ALT     0x14

#define GT911_INT_GPIO     GPIO_NUM_4

#define GT911_REG_STATUS      0x814E
#define GT911_REG_FIRST_POINT 0x814F
#define GT911_REG_PRODUCT_ID  0x8140
#define GT911_REG_CONFIG_VER  0x8047

static uint8_t s_touch_addr = 0;
static bool s_touch_ready = false;

static bool gt911_hw_reset_sequence(void)
{
    const gpio_config_t int_cfg = {
        .pin_bit_mask = BIT64(GT911_INT_GPIO),
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };

    if (gpio_config(&int_cfg) != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to configure GT911 INT GPIO");
        return false;
    }

    if (!hmi_7b_ioexp_set_output_bit(1, false))
    {
        ESP_LOGW(TAG, "Failed to hold GT911 reset low");
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    if (gpio_set_level(GT911_INT_GPIO, 0) != ESP_OK)
    {
        ESP_LOGW(TAG, "Failed to drive GT911 INT low");
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(100));

    if (!hmi_7b_ioexp_set_output_bit(1, true))
    {
        ESP_LOGW(TAG, "Failed to release GT911 reset");
        return false;
    }
    vTaskDelay(pdMS_TO_TICKS(200));

    return true;
}

static void gt911_log_id(uint8_t addr)
{
    uint8_t id[3] = {0};
    uint8_t cfg_ver = 0;

    if (hmi_7b_bus_read_reg16(addr, GT911_REG_PRODUCT_ID, id, sizeof(id)))
    {
        ESP_LOGI(TAG, "GT911 Product ID at 0x%02X: %02X %02X %02X", addr, id[0], id[1], id[2]);
    }
    if (hmi_7b_bus_read_reg16(addr, GT911_REG_CONFIG_VER, &cfg_ver, 1))
    {
        ESP_LOGI(TAG, "GT911 Config version: %u", (unsigned)cfg_ver);
    }
}

static bool gt911_probe_addr(void)
{
    if (hmi_7b_bus_probe(GT911_ADDR_PRIMARY))
    {
        s_touch_addr = GT911_ADDR_PRIMARY;
        return true;
    }

    if (hmi_7b_bus_probe(GT911_ADDR_ALT))
    {
        s_touch_addr = GT911_ADDR_ALT;
        return true;
    }

    s_touch_addr = 0;
    return false;
}

bool hmi_7b_touch_detect(void)
{
    if (!hmi_7b_bus_init())
    {
        return false;
    }

    return gt911_probe_addr();
}

bool hmi_7b_touch_init(void)
{
    if (s_touch_ready)
    {
        return true;
    }

    if (!hmi_7b_ioexp_init())
    {
        ESP_LOGW(TAG, "IO expander init failed before touch reset");
        return false;
    }

    if (!gt911_hw_reset_sequence())
    {
        ESP_LOGW(TAG, "GT911 hardware reset sequence failed");
        return false;
    }

    if (!gt911_probe_addr())
    {
        ESP_LOGW(TAG, "GT911 not detected on 0x%02X or 0x%02X", GT911_ADDR_PRIMARY, GT911_ADDR_ALT);
        return false;
    }

    s_touch_ready = true;
    gt911_log_id(s_touch_addr);
    ESP_LOGI(TAG, "GT911 ready at 0x%02X", s_touch_addr);
    return true;
}

bool hmi_7b_touch_read(uint16_t *x, uint16_t *y, bool *pressed)
{
    if (x == NULL || y == NULL || pressed == NULL)
    {
        return false;
    }

    *x = 0;
    *y = 0;
    *pressed = false;

    if (!s_touch_ready || s_touch_addr == 0)
    {
        return false;
    }

    uint8_t status = 0;
    if (!hmi_7b_bus_read_reg16(s_touch_addr, GT911_REG_STATUS, &status, 1))
    {
        return false;
    }

    if ((status & 0x80U) == 0)
    {
        return true;
    }

    uint8_t points = status & 0x0FU;
    if (points > 0)
    {
        uint8_t raw[8] = {0};
        if (!hmi_7b_bus_read_reg16(s_touch_addr, GT911_REG_FIRST_POINT, raw, sizeof(raw)))
        {
            return false;
        }

        *x = (uint16_t)(((uint16_t)raw[1] << 8) | raw[0]);
        *y = (uint16_t)(((uint16_t)raw[3] << 8) | raw[2]);
        *pressed = true;
    }

    uint8_t clear = 0;
    (void)hmi_7b_bus_write_reg16(s_touch_addr, GT911_REG_STATUS, &clear, 1);
    return true;
}
