#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char *TAG = "zic_main";

void app_main(void)
{
    ESP_LOGI(TAG, "Zmartify Irrigation Controller booting");

    while (1) {
        ESP_LOGI(TAG, "System heartbeat");
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
