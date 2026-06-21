#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "irrigation_engine.h"
#include "mqtt_topic.h"

static const char *TAG = "zic_main";

void app_main(void)
{
    irrigation_engine_t engine;
    char state_topic[ZIC_MQTT_TOPIC_MAX];

    irrigation_engine_init(&engine);
    zic_mqtt_topic_build("state", state_topic, sizeof(state_topic));

    ESP_LOGI(TAG, "Zmartify Irrigation Controller booting");
    irrigation_engine_start_zone(&engine, 1, 300);

    while (1) {
        ESP_LOGI(TAG,
                 "System heartbeat. controller_state=%d active_zone=%d publish_topic=%s",
                 (int)engine.controller.state,
                 (int)engine.controller.active_zone,
                 state_topic);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
