#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "alarm_manager.h"
#include "et_engine.h"
#include "flow_manager.h"
#include "irrigation_engine.h"
#include "mqtt_topic.h"
#include "pressure_manager.h"
#include "storage_manager.h"
#include "weather_manager.h"

static const char *TAG = "zic_main";
static zic_log_entry_t s_log_buffer[128];

void app_main(void)
{
    irrigation_engine_t engine;
    alarm_manager_t alarm_manager;
    flow_manager_t flow_manager;
    pressure_manager_t pressure_manager;
    storage_manager_t storage_manager;
    weather_snapshot_t weather_snapshot;
    weather_decision_t weather_decision;
    et_output_t et_output;
    char state_topic[ZIC_MQTT_TOPIC_MAX];
    char csv_line[128];

    irrigation_engine_init(&engine);
    alarm_manager_init(&alarm_manager);
    flow_manager_init(&flow_manager);
    pressure_manager_init(&pressure_manager, 2000, 7000);
    storage_manager_init(&storage_manager, s_log_buffer, 128);
    flow_manager_set_baseline(&flow_manager, 1234);
    weather_snapshot.rain_mm_last_24h = 0.0f;
    weather_snapshot.rain_probability_pct = 20.0f;
    weather_snapshot.humidity_pct = 55.0f;
    weather_snapshot.uv_index = 8.0f;
    weather_snapshot.temperature_c = 33.0f;
    weather_snapshot.wind_speed_mps = 3.0f;
    zic_mqtt_topic_build("state", state_topic, sizeof(state_topic));

    ESP_LOGI(TAG, "Zmartify Irrigation Controller booting");
    irrigation_engine_start_zone(&engine, 1, 300);

    while (1) {
        flow_manager_update(&flow_manager, 1400, &alarm_manager);
        pressure_manager_update(&pressure_manager, 3500, &alarm_manager);
        storage_manager_append(&storage_manager, 1, ZIC_LOG_IRRIGATION, "heartbeat");
        storage_manager_export_csv(&s_log_buffer[0], csv_line, sizeof(csv_line));
        weather_manager_evaluate(&weather_snapshot, &weather_decision);

        et_input_t et_input = {
            .temperature_c = weather_snapshot.temperature_c,
            .humidity_pct = weather_snapshot.humidity_pct,
            .wind_speed_mps = weather_snapshot.wind_speed_mps,
            .solar_radiation_mj_m2 = 18.0f,
        };
        et_engine_compute(&et_input, &et_output);

        ESP_LOGI(TAG,
                 "System heartbeat. controller_state=%d active_zone=%d alarm_high_flow=%d et_daily=%.2f increase=%d logs=%u first_log=%s publish_topic=%s",
                 (int)engine.controller.state,
                 (int)engine.controller.active_zone,
                 alarm_manager_is_active(&alarm_manager, ZIC_ALARM_HIGH_FLOW),
                 et_output.daily_et_mm,
                 weather_decision.increase_watering,
                 (unsigned)storage_manager_count(&storage_manager),
                 csv_line,
                 state_topic);
        vTaskDelay(pdMS_TO_TICKS(10000));
    }
}
