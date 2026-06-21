#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "alarm_manager.h"
#include "flow_manager.h"
#include "irrigation_engine.h"
#include "mqtt_topic.h"
#include "pressure_manager.h"

static const char *TAG = "zic_main";

void app_main(void)
{
    irrigation_engine_t engine;
    alarm_manager_t alarm_manager;
    flow_manager_t flow_manager;
    pressure_manager_t pressure_manager;
    char state_topic[ZIC_MQTT_TOPIC_MAX];

    irrigation_engine_init(&engine);
    alarm_manager_init(&alarm_manager);
    flow_manager_init(&flow_manager);
    pressure_manager_init(&pressure_manager, 2000, 7000);
    flow_manager_set_baseline(&flow_manager, 1234);
    zic_mqtt_topic_build("state", state_topic, sizeof(state_topic));

    ESP_LOGI(TAG, "Zmartify Irrigation Controller booting");
    irrigation_engine_start_zone(&engine, 1, 300);

    while (1) {
        flow_manager_update(&flow_manager, 1400, &alarm_manager);
        pressure_manager_update(&pressure_manager, 3500, &alarm_manager);

        ESP_LOGI(TAG,
                 "System heartbeat. controller_state=%d active_zone=%d alarm_high_flow=%d publish_topic=%s",
                 (int)engine.controller.state,
                 (int)engine.controller.active_zone,
                 alarm_manager_is_active(&alarm_manager, ZIC_ALARM_HIGH_FLOW),
                 state_topic);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
