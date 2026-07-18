#include "esp_log.h"
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "alarm_manager.h"
#include "et_engine.h"
#include "flow_manager.h"
#include "irrigation_engine.h"
#include "mqtt_topic.h"
#include "mqtt_transport.h"
#include "ota_manager.h"
#include "persistent_store.h"
#include "pressure_manager.h"
#include "storage_manager.h"
#include "weather_manager.h"
#include "hmi_board.h"
#include "zic_v2.h"
#include <time.h>

static const char *TAG = "zic_main";
static zic_log_entry_t s_log_buffer[128];

#define ZIC_CMD_QUEUE_LEN 16
#define ZIC_CTRL_TASK_STACK 6144
#define ZIC_TELEM_TASK_STACK 4096
#define ZIC_CTRL_TASK_PRIO 8
#define ZIC_TELEM_TASK_PRIO 5

#define ZIC_EVENT_ENGINE_READY BIT0
#define ZIC_EVENT_FAULT BIT1

#define ZIC_DEFAULT_BROKER_URI "mqtt://192.168.10.2:1883"
#define ZIC_DEFAULT_CLIENT_ID "zic-controller-01"

/* Device identity for Zmartify v2 topics; must match the edge registry
 * device_id assigned during onboarding (lowercase, hyphenated). */
#define ZIC_V2_DEVICE_ID "zmartify-irrigation-01"

typedef enum {
    ZIC_CMD_START_ZONE = 0,
    ZIC_CMD_STOP_ZONE,
    ZIC_CMD_STOP_ALL,
    ZIC_CMD_SET_RAIN_DELAY,
    ZIC_CMD_CLEAR_RAIN_DELAY,
    ZIC_CMD_TRIGGER_OTA
} zic_runtime_command_type_t;

typedef struct {
    zic_runtime_command_type_t type;
    uint8_t zone_id;
    uint32_t runtime_seconds;
    uint16_t rain_delay_hours;
    char ota_url[160];
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
    mqtt_transport_t mqtt_transport;
} zic_app_context_t;

static zic_app_context_t s_ctx;
static QueueHandle_t s_command_queue;
static EventGroupHandle_t s_runtime_events;
static SemaphoreHandle_t s_ctx_lock;

static const char *s_command_topics[] = {
    ZIC_MQTT_ROOT "/command/start_zone",
    ZIC_MQTT_ROOT "/command/stop_zone",
    ZIC_MQTT_ROOT "/command/run_program",
    ZIC_MQTT_ROOT "/command/stop_all",
    ZIC_MQTT_ROOT "/command/rain_delay",
    "zmartify/v2/devices/" ZIC_V2_DEVICE_ID "/commands/irrigation/#",
};

static char s_v2_state_topic[128];
static char s_v2_outcome_topic[128];
static char s_v2_last_command_id[40];

static void zic_v2_now_iso(char *out, size_t out_len)
{
    time_t now = time(NULL);
    struct tm tm_utc;
    gmtime_r(&now, &tm_utc);
    (void)strftime(out, out_len, "%Y-%m-%dT%H:%M:%SZ", &tm_utc);
}

static size_t zic_copy_to_cstr(char *dst, size_t dst_len, const char *src, size_t src_len)
{
    size_t len;

    if (dst == NULL || dst_len == 0 || src == NULL) {
        return 0;
    }

    len = src_len;
    if (len >= dst_len) {
        len = dst_len - 1;
    }

    memcpy(dst, src, len);
    dst[len] = '\0';
    return len;
}

static bool zic_parse_u32_payload(const char *payload, size_t payload_len, uint32_t *value_out)
{
    char buf[32];
    char *end;
    unsigned long v;

    if (value_out == NULL || payload == NULL || payload_len == 0) {
        return false;
    }

    zic_copy_to_cstr(buf, sizeof(buf), payload, payload_len);
    v = strtoul(buf, &end, 10);
    if (end == buf) {
        return false;
    }

    *value_out = (uint32_t)v;
    return true;
}

static cJSON *zic_parse_json_payload(const char *payload,
                                     size_t payload_len,
                                     char *scratch,
                                     size_t scratch_len)
{
    if (payload == NULL || scratch == NULL || scratch_len == 0 || payload_len == 0) {
        return NULL;
    }

    zic_copy_to_cstr(scratch, scratch_len, payload, payload_len);
    return cJSON_Parse(scratch);
}

static bool zic_get_json_u32(const cJSON *root, const char *key, uint32_t *value_out)
{
    const cJSON *item;

    if (root == NULL || key == NULL || value_out == NULL) {
        return false;
    }

    item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (!cJSON_IsNumber(item) || item->valuedouble < 0) {
        return false;
    }

    *value_out = (uint32_t)item->valuedouble;
    return true;
}

static void zic_mqtt_on_message(const char *topic,
                                size_t topic_len,
                                const char *payload,
                                size_t payload_len,
                                void *user_ctx)
{
    zic_runtime_command_t cmd;
    zic_mqtt_command_t mqtt_cmd;
    uint32_t value;
    uint32_t runtime_seconds;
    char topic_buf[ZIC_MQTT_TOPIC_MAX];
    char json_scratch[256];
    cJSON *json = NULL;

    (void)user_ctx;
    if (s_command_queue == NULL || topic == NULL) {
        return;
    }

    memset(&cmd, 0, sizeof(cmd));
    value = 0;
    runtime_seconds = 300;
    zic_copy_to_cstr(topic_buf, sizeof(topic_buf), topic, topic_len);

    /* Zmartify v2 command envelope: route by trailing segment and unwrap
     * command_id + parameters before dispatching to the same command queue. */
    const char *v2_prefix = "zmartify/v2/devices/" ZIC_V2_DEVICE_ID "/commands/irrigation/";
    if (strncmp(topic_buf, v2_prefix, strlen(v2_prefix)) == 0) {
        const char *action = topic_buf + strlen(v2_prefix);
        json = zic_parse_json_payload(payload, payload_len, json_scratch, sizeof(json_scratch));
        if (json == NULL) {
            ESP_LOGW(TAG, "Invalid v2 command payload for %s", action);
            return;
        }

        const cJSON *command_id = cJSON_GetObjectItemCaseSensitive(json, "command_id");
        if (cJSON_IsString(command_id) && command_id->valuestring != NULL) {
            strncpy(s_v2_last_command_id, command_id->valuestring, sizeof(s_v2_last_command_id) - 1U);
            s_v2_last_command_id[sizeof(s_v2_last_command_id) - 1U] = '\0';
        }

        const cJSON *parameters = cJSON_GetObjectItemCaseSensitive(json, "parameters");
        const cJSON *scope = cJSON_IsObject(parameters) ? parameters : json;

        bool queued = false;
        if (strcmp(action, "zone/start") == 0 || strcmp(action, "start_zone") == 0) {
            if (zic_get_json_u32(scope, "zone_id", &value) && value >= 1 && value <= ZIC_MAX_ZONES) {
                cmd.type = ZIC_CMD_START_ZONE;
                cmd.zone_id = (uint8_t)value;
                cmd.runtime_seconds = 300;
                if (zic_get_json_u32(scope, "duration_seconds", &runtime_seconds) ||
                    zic_get_json_u32(scope, "runtime_seconds", &runtime_seconds)) {
                    cmd.runtime_seconds = runtime_seconds;
                }
                queued = true;
            }
        } else if (strcmp(action, "zone/stop") == 0 || strcmp(action, "stop_zone") == 0) {
            if (zic_get_json_u32(scope, "zone_id", &value) && value >= 1 && value <= ZIC_MAX_ZONES) {
                cmd.type = ZIC_CMD_STOP_ZONE;
                cmd.zone_id = (uint8_t)value;
                queued = true;
            }
        } else if (strcmp(action, "stop_all") == 0) {
            cmd.type = ZIC_CMD_STOP_ALL;
            queued = true;
        } else if (strcmp(action, "rain_delay") == 0) {
            if (zic_get_json_u32(scope, "delay_hours", &value) || zic_get_json_u32(scope, "hours", &value)) {
                if (value == 0) {
                    cmd.type = ZIC_CMD_CLEAR_RAIN_DELAY;
                } else {
                    cmd.type = ZIC_CMD_SET_RAIN_DELAY;
                    cmd.rain_delay_hours = (uint16_t)value;
                }
                queued = true;
            }
        }

        cJSON_Delete(json);
        if (queued) {
            xQueueSend(s_command_queue, &cmd, 0);
        } else {
            ESP_LOGW(TAG, "Unsupported or invalid v2 command: %s", action);
        }
        return;
    }

    if (!zic_mqtt_command_from_topic(topic_buf, &mqtt_cmd)) {
        return;
    }

    switch (mqtt_cmd) {
    case ZIC_MQTT_CMD_START_ZONE:
        if (!zic_parse_u32_payload(payload, payload_len, &value)) {
            json = zic_parse_json_payload(payload, payload_len, json_scratch, sizeof(json_scratch));
            if (json == NULL) {
                ESP_LOGW(TAG, "Invalid start_zone payload");
                return;
            }

            if (!zic_get_json_u32(json, "zone", &value) && !zic_get_json_u32(json, "zone_id", &value)) {
                cJSON_Delete(json);
                ESP_LOGW(TAG, "start_zone requires zone or zone_id");
                return;
            }

            if (zic_get_json_u32(json, "runtime_seconds", &runtime_seconds) ||
                zic_get_json_u32(json, "runtime", &runtime_seconds)) {
                cmd.runtime_seconds = runtime_seconds;
            }

            cJSON_Delete(json);
        }

        if (value == 0 || value > ZIC_MAX_ZONES) {
            ESP_LOGW(TAG, "start_zone zone out of range: %lu", (unsigned long)value);
            return;
        }

        cmd.type = ZIC_CMD_START_ZONE;
        cmd.zone_id = (uint8_t)value;
        if (cmd.runtime_seconds == 0) {
            cmd.runtime_seconds = 300;
        }
        break;

    case ZIC_MQTT_CMD_STOP_ZONE:
        if (!zic_parse_u32_payload(payload, payload_len, &value)) {
            json = zic_parse_json_payload(payload, payload_len, json_scratch, sizeof(json_scratch));
            if (json == NULL) {
                ESP_LOGW(TAG, "Invalid stop_zone payload");
                return;
            }

            if (!zic_get_json_u32(json, "zone", &value) && !zic_get_json_u32(json, "zone_id", &value)) {
                cJSON_Delete(json);
                ESP_LOGW(TAG, "stop_zone requires zone or zone_id");
                return;
            }
            cJSON_Delete(json);
        }

        if (value == 0 || value > ZIC_MAX_ZONES) {
            ESP_LOGW(TAG, "stop_zone zone out of range: %lu", (unsigned long)value);
            return;
        }

        cmd.type = ZIC_CMD_STOP_ZONE;
        cmd.zone_id = (uint8_t)value;
        break;

    case ZIC_MQTT_CMD_STOP_ALL:
        cmd.type = ZIC_CMD_STOP_ALL;
        break;

    case ZIC_MQTT_CMD_RAIN_DELAY:
        if (!zic_parse_u32_payload(payload, payload_len, &value)) {
            json = zic_parse_json_payload(payload, payload_len, json_scratch, sizeof(json_scratch));
            if (json == NULL) {
                ESP_LOGW(TAG, "Invalid rain_delay payload");
                return;
            }

            if (!zic_get_json_u32(json, "hours", &value) &&
                !zic_get_json_u32(json, "rain_delay_hours", &value)) {
                cJSON_Delete(json);
                ESP_LOGW(TAG, "rain_delay requires hours or rain_delay_hours");
                return;
            }
            cJSON_Delete(json);
        }

        if (value == 0) {
            cmd.type = ZIC_CMD_CLEAR_RAIN_DELAY;
        } else {
            cmd.type = ZIC_CMD_SET_RAIN_DELAY;
            cmd.rain_delay_hours = (uint16_t)value;
        }
        break;

    case ZIC_MQTT_CMD_RUN_PROGRAM:
        return;
    default:
        return;
    }

    xQueueSend(s_command_queue, &cmd, 0);
}

static void zic_runtime_init_context(zic_app_context_t *ctx)
{
    uint32_t baseline_flow = 1234;

    persistent_store_init();
    persistent_store_get_u32("flow_baseline", &baseline_flow, 1234);

    irrigation_engine_init(&ctx->engine);
    alarm_manager_init(&ctx->alarm_manager);
    flow_manager_init(&ctx->flow_manager);
    pressure_manager_init(&ctx->pressure_manager, 2000, 7000);
    storage_manager_init(&ctx->storage_manager, s_log_buffer, 128);
    flow_manager_set_baseline(&ctx->flow_manager, baseline_flow);

    ctx->weather_snapshot.rain_mm_last_24h = 0.0f;
    ctx->weather_snapshot.rain_probability_pct = 20.0f;
    ctx->weather_snapshot.humidity_pct = 55.0f;
    ctx->weather_snapshot.uv_index = 8.0f;
    ctx->weather_snapshot.temperature_c = 33.0f;
    ctx->weather_snapshot.wind_speed_mps = 3.0f;
    zic_mqtt_topic_build("state", ctx->state_topic, sizeof(ctx->state_topic));
    zic_v2_state_topic(ZIC_V2_DEVICE_ID, s_v2_state_topic, sizeof(s_v2_state_topic));
    zic_v2_outcome_topic(ZIC_V2_DEVICE_ID, s_v2_outcome_topic, sizeof(s_v2_outcome_topic));

    mqtt_transport_config_t mqtt_cfg = {
        .broker_uri = ZIC_DEFAULT_BROKER_URI,
        .client_id = ZIC_DEFAULT_CLIENT_ID,
        .username = NULL,
        .password = NULL,
        .subscribe_topics = s_command_topics,
        .subscribe_topic_count = sizeof(s_command_topics) / sizeof(s_command_topics[0]),
        .on_message = zic_mqtt_on_message,
        .user_ctx = NULL,
    };
    if (mqtt_transport_init(&ctx->mqtt_transport, &mqtt_cfg)) {
        mqtt_transport_start(&ctx->mqtt_transport);
    }
}

static void zic_publish_v2_outcome(zic_app_context_t *ctx,
                                   const char *event_type,
                                   const char *severity,
                                   const char *result,
                                   const char *detail,
                                   int zone_id)
{
    char stamp[32];
    char payload[512];

    if (!mqtt_transport_is_connected(&ctx->mqtt_transport)) {
        return;
    }

    zic_v2_now_iso(stamp, sizeof(stamp));
    if (!zic_v2_build_outcome(payload, sizeof(payload), stamp, event_type, severity, result, detail,
                              s_v2_last_command_id[0] != '\0' ? s_v2_last_command_id : NULL,
                              NULL, zone_id)) {
        return;
    }
    (void)mqtt_transport_publish(&ctx->mqtt_transport, s_v2_outcome_topic, payload, 1, false);
}

static void zic_runtime_apply_command(zic_app_context_t *ctx, const zic_runtime_command_t *cmd)
{
    switch (cmd->type) {
    case ZIC_CMD_START_ZONE:
        irrigation_engine_start_zone(&ctx->engine, cmd->zone_id, cmd->runtime_seconds);
        zic_publish_v2_outcome(ctx, "run.started", "info", "accepted", NULL, cmd->zone_id);
        break;
    case ZIC_CMD_STOP_ZONE:
        irrigation_engine_stop_zone(&ctx->engine, cmd->zone_id);
        zic_publish_v2_outcome(ctx, "run.stopped", "info", "completed", NULL, cmd->zone_id);
        break;
    case ZIC_CMD_STOP_ALL:
        irrigation_engine_stop_all(&ctx->engine);
        zic_publish_v2_outcome(ctx, "run.stopped", "info", "completed", "stop_all", 0);
        break;
    case ZIC_CMD_SET_RAIN_DELAY:
        persistent_store_set_u32("rain_delay_h", cmd->rain_delay_hours);
        zic_controller_apply_event(&ctx->engine.controller, ZIC_EV_RAIN_DELAY_SET, -1);
        zic_publish_v2_outcome(ctx, "rain.delay_set", "info", "accepted", NULL, 0);
        break;
    case ZIC_CMD_CLEAR_RAIN_DELAY:
        persistent_store_set_u32("rain_delay_h", 0);
        zic_controller_apply_event(&ctx->engine.controller, ZIC_EV_RAIN_DELAY_CLEAR, -1);
        zic_publish_v2_outcome(ctx, "rain.delay_cleared", "info", "accepted", NULL, 0);
        break;
    case ZIC_CMD_TRIGGER_OTA: {
        ota_manager_config_t ota_cfg = {
            .firmware_url = cmd->ota_url,
            .cert_pem = NULL,
        };
        if (cmd->ota_url[0] != '\0') {
            ota_manager_perform(&ota_cfg);
        }
        break;
    }
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
        persistent_store_set_u32("flow_baseline", s_ctx.flow_manager.baseline_lpm_x100);
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
                 "Telemetry controller_state=%d active_zone=%d alarm_high_flow=%d et_daily=%.2f fault=%d mqtt=%d logs=%u first_log=%s publish_topic=%s",
                 (int)s_ctx.engine.controller.state,
                 (int)s_ctx.engine.controller.active_zone,
                 alarm_manager_is_active(&s_ctx.alarm_manager, ZIC_ALARM_HIGH_FLOW),
                 s_ctx.et_output.daily_et_mm,
                 (bits & ZIC_EVENT_FAULT) != 0,
                 mqtt_transport_is_connected(&s_ctx.mqtt_transport),
                 (unsigned)storage_manager_count(&s_ctx.storage_manager),
                 csv_line,
                 s_ctx.state_topic);

        if (mqtt_transport_is_connected(&s_ctx.mqtt_transport)) {
            mqtt_transport_publish(&s_ctx.mqtt_transport,
                                   s_ctx.state_topic,
                                   csv_line,
                                   ZIC_MQTT_QOS_TELEMETRY,
                                   false);

            /* Zmartify v2 reported-state (schema mqtt-v2/reported-state). */
            char stamp[32];
            char v2_payload[512];
            zic_v2_now_iso(stamp, sizeof(stamp));
            zic_v2_hydraulics_t hyd = {
                .flow_lpm = (double)s_ctx.flow_manager.current_lpm_x100 / 100.0,
                .pressure_bar = (double)s_ctx.pressure_manager.current_pressure_mbar / 1000.0,
                .water_liters = -1.0,
            };
            zic_v2_weather_t wx = {
                .temperature_c = (double)s_ctx.weather_snapshot.temperature_c,
                .rain_mm = (double)s_ctx.weather_snapshot.rain_mm_last_24h,
                .wind_mps = (double)s_ctx.weather_snapshot.wind_speed_mps,
                .eto_mm = (double)s_ctx.et_output.daily_et_mm,
            };
            if (zic_v2_build_reported_state(v2_payload, sizeof(v2_payload), stamp, NULL, &hyd, NULL, &wx)) {
                mqtt_transport_publish(&s_ctx.mqtt_transport,
                                       s_v2_state_topic,
                                       v2_payload,
                                       ZIC_MQTT_QOS_TELEMETRY,
                                       false);
            }
        }
        xSemaphoreGive(s_ctx_lock);

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

void app_main(void)
{
    hmi_board_status_t hmi_status = {0};

    ESP_LOGI(TAG, "Zmartify Irrigation Controller booting");

    if (!hmi_board_init()) {
        ESP_LOGW(TAG, "HMI init reported degraded status");
    }
    hmi_board_get_status(&hmi_status);
    ESP_LOGI(TAG,
             "HMI status: panel=%d touch=%d backlight=%d",
             hmi_status.panel_ready,
             hmi_status.touch_present,
             hmi_status.backlight_enabled);

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
