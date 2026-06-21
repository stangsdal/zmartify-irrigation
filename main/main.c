#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_topic.h"
#include "zic_state_machine.h"

static const char *TAG = "zic_main";

void app_main(void)
{
    zic_controller_t controller;
    char state_topic[ZIC_MQTT_TOPIC_MAX];

    zic_controller_init(&controller);
    zic_mqtt_topic_build("state", state_topic, sizeof(state_topic));

    ESP_LOGI(TAG, "Zmartify Irrigation Controller booting");
    zic_controller_apply_event(&controller, ZIC_EV_BOOT_DONE, -1);
    zic_controller_apply_event(&controller, ZIC_EV_INIT_DONE, -1);

    while (1) {
        ESP_LOGI(TAG, "System heartbeat. controller_state=%d publish_topic=%s", (int)controller.state, state_topic);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
