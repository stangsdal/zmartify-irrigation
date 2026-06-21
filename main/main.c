#include "esp_log.h"
#include "freertos/event_groups.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
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

#define ZIC_CMD_QUEUE_LEN 16
#define ZIC_CTRL_TASK_STACK 6144
#define ZIC_TELEM_TASK_STACK 4096
#define ZIC_CTRL_TASK_PRIO 8
#define ZIC_TELEM_TASK_PRIO 5

#define ZIC_EVENT_ENGINE_READY BIT0
#define ZIC_EVENT_FAULT BIT1

typedef enum {
    ZIC_CMD_START_ZONE = 0,
    ZIC_CMD_STOP_ZONE,
    ZIC_CMD_STOP_ALL,
    ZIC_CMD_SET_RAIN_DELAY,
    ZIC_CMD_CLEAR_RAIN_DELAY
} zic_runtime_command_type_t;

typedef struct {
    zic_runtime_command_type_t type;
    uint8_t zone_id;
    uint32_t runtime_seconds;
    uint16_t rain_delay_hours;
} zic_runtime_command_t;

typedef struct {
    irrigation_engine_t engine;
    alarm_manager_t alarm_manager;
    flow_manager_t flow_manager;
    pressure_manager_t pressure_manager;
    storage_manager_t storage_manager;
    weather_snapshot_t weather_snapshot;
    weather_decision_t weather_decision;
    et_output_t et_output;
    char state_topic[ZIC_MQTT_TOPIC_MAX];
} zic_app_context_t;

static zic_app_context_t s_ctx;
static QueueHandle_t s_command_queue;
static EventGroupHandle_t s_runtime_events;
static SemaphoreHandle_t s_ctx_lock;

static void zic_runtime_init_context(zic_app_context_t *ctx)
{
    irrigation_engine_init(&ctx->engine);
    alarm_manager_init(&ctx->alarm_manager);
    flow_manager_init(&ctx->flow_manager);
    pressure_manager_init(&ctx->pressure_manager, 2000, 7000);
    storage_manager_init(&ctx->storage_manager, s_log_buffer, 128);
    flow_manager_set_baseline(&ctx->flow_manager, 1234);

    ctx->weather_snapshot.rain_mm_last_24h = 0.0f;
    ctx->weather_snapshot.rain_probability_pct = 20.0f;
    ctx->weather_snapshot.humidity_pct = 55.0f;
    ctx->weather_snapshot.uv_index = 8.0f;
    ctx->weather_snapshot.temperature_c = 33.0f;
    ctx->weather_snapshot.wind_speed_mps = 3.0f;
    zic_mqtt_topic_build("state", ctx->state_topic, sizeof(ctx->state_topic));
}

static void zic_runtime_apply_command(zic_app_context_t *ctx, const zic_runtime_command_t *cmd)
{
    switch (cmd->type) {
    case ZIC_CMD_START_ZONE:
        irrigation_engine_start_zone(&ctx->engine, cmd->zone_id, cmd->runtime_seconds);
        break;
    case ZIC_CMD_STOP_ZONE:
        irrigation_engine_stop_zone(&ctx->engine, cmd->zone_id);
        break;
    case ZIC_CMD_STOP_ALL:
        irrigation_engine_stop_all(&ctx->engine);
        break;
    case ZIC_CMD_SET_RAIN_DELAY:
        (void)cmd->rain_delay_hours;
        zic_controller_apply_event(&ctx->engine.controller, ZIC_EV_RAIN_DELAY_SET, -1);
        break;
    case ZIC_CMD_CLEAR_RAIN_DELAY:
        zic_controller_apply_event(&ctx->engine.controller, ZIC_EV_RAIN_DELAY_CLEAR, -1);
        break;
    default:
        break;
    }
}

static void zic_control_task(void *arg)
{
    (void)arg;

    zic_runtime_command_t cmd;
    xEventGroupSetBits(s_runtime_events, ZIC_EVENT_ENGINE_READY);

    for (;;) {
        if (xQueueReceive(s_command_queue, &cmd, pdMS_TO_TICKS(1000)) == pdTRUE) {
            xSemaphoreTake(s_ctx_lock, portMAX_DELAY);
            zic_runtime_apply_command(&s_ctx, &cmd);
            xSemaphoreGive(s_ctx_lock);
        }

        xSemaphoreTake(s_ctx_lock, portMAX_DELAY);

        flow_manager_update(&s_ctx.flow_manager, 1400, &s_ctx.alarm_manager);
        pressure_manager_update(&s_ctx.pressure_manager, 3500, &s_ctx.alarm_manager);
        storage_manager_append(&s_ctx.storage_manager, 1, ZIC_LOG_IRRIGATION, "heartbeat");
        weather_manager_evaluate(&s_ctx.weather_snapshot, &s_ctx.weather_decision);

        et_input_t et_input = {
            .temperature_c = s_ctx.weather_snapshot.temperature_c,
            .humidity_pct = s_ctx.weather_snapshot.humidity_pct,
            .wind_speed_mps = s_ctx.weather_snapshot.wind_speed_mps,
            .solar_radiation_mj_m2 = 18.0f,
        };
        et_engine_compute(&et_input, &s_ctx.et_output);

        if (alarm_manager_is_active(&s_ctx.alarm_manager, ZIC_ALARM_LEAK_DETECTED)) {
            xEventGroupSetBits(s_runtime_events, ZIC_EVENT_FAULT);
        } else {
            xEventGroupClearBits(s_runtime_events, ZIC_EVENT_FAULT);
        }

        xSemaphoreGive(s_ctx_lock);
    }
}

static void zic_telemetry_task(void *arg)
{
    (void)arg;
    char csv_line[128] = {0};

    xEventGroupWaitBits(s_runtime_events, ZIC_EVENT_ENGINE_READY, pdFALSE, pdTRUE, portMAX_DELAY);

    for (;;) {
        EventBits_t bits = xEventGroupGetBits(s_runtime_events);

        xSemaphoreTake(s_ctx_lock, portMAX_DELAY);
        if (storage_manager_count(&s_ctx.storage_manager) > 0) {
            storage_manager_export_csv(&s_log_buffer[0], csv_line, sizeof(csv_line));
        } else {
            csv_line[0] = '\0';
        }

        ESP_LOGI(TAG,
                 "Telemetry controller_state=%d active_zone=%d alarm_high_flow=%d et_daily=%.2f fault=%d logs=%u first_log=%s publish_topic=%s",
                 (int)s_ctx.engine.controller.state,
                 (int)s_ctx.engine.controller.active_zone,
                 alarm_manager_is_active(&s_ctx.alarm_manager, ZIC_ALARM_HIGH_FLOW),
                 s_ctx.et_output.daily_et_mm,
                 (bits & ZIC_EVENT_FAULT) != 0,
                 (unsigned)storage_manager_count(&s_ctx.storage_manager),
                 csv_line,
                 s_ctx.state_topic);
        xSemaphoreGive(s_ctx_lock);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void)
{
    ESP_LOGI(TAG, "Zmartify Irrigation Controller booting");

    s_command_queue = xQueueCreate(ZIC_CMD_QUEUE_LEN, sizeof(zic_runtime_command_t));
    s_runtime_events = xEventGroupCreate();
    s_ctx_lock = xSemaphoreCreateMutex();
    if (s_command_queue == NULL || s_runtime_events == NULL || s_ctx_lock == NULL) {
        ESP_LOGE(TAG, "Failed to allocate runtime resources");
        return;
    }

    xSemaphoreTake(s_ctx_lock, portMAX_DELAY);
    zic_runtime_init_context(&s_ctx);
    xSemaphoreGive(s_ctx_lock);

    xTaskCreate(zic_control_task, "zic_ctrl", ZIC_CTRL_TASK_STACK, NULL, ZIC_CTRL_TASK_PRIO, NULL);
    xTaskCreate(zic_telemetry_task, "zic_telem", ZIC_TELEM_TASK_STACK, NULL, ZIC_TELEM_TASK_PRIO, NULL);

    zic_runtime_command_t initial_cmd = {
        .type = ZIC_CMD_START_ZONE,
        .zone_id = 1,
        .runtime_seconds = 300,
        .rain_delay_hours = 0,
    };
    xQueueSend(s_command_queue, &initial_cmd, 0);
}
