#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_http_server.h"
#include "esp_mac.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "nvs_flash.h"
#include <stdlib.h>
#include <string.h>
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "freertos/semphr.h"
#include "freertos/task.h"
#include "alarm_manager.h"
#include "config_manager.h"
#include "diagnostics_manager.h"
#include "et_engine.h"
#include "event_bus.h"
#include "flow_manager.h"
#include "hal.h"
#include "irrigation_engine.h"
#include "mqtt_topic.h"
#include "mqtt_transport.h"
#include "ota_manager.h"
#include "persistent_store.h"
#include "pressure_manager.h"
#include "relay_manager.h"
#include "storage_manager.h"
#include "valve_diagnostics.h"
#include "weather_manager.h"
#include "watersensor_client.h"
#include "hmi_board.h"
#include "zic_v2.h"
#include <time.h>

#if __has_include("wifi_credentials.local.h")
#include "wifi_credentials.local.h"
#else
static const uint8_t zic_wifi_ssid[] = {0};
static const uint8_t zic_wifi_password[] = {0};
#endif

#ifndef ZIC_RELAY_SELF_TEST
#define ZIC_RELAY_SELF_TEST 0
#endif

#ifndef ZIC_OTA_FORCE_HEALTH_FAILURE
#define ZIC_OTA_FORCE_HEALTH_FAILURE 0
#endif

static const char *TAG = "zic_main";
static zic_log_entry_t s_log_buffer[ZIC_LOG_PERSIST_CAPACITY];

#define ZIC_CMD_QUEUE_LEN 16
#define ZIC_CTRL_TASK_STACK 6144
#define ZIC_TELEM_TASK_STACK 4096
#define ZIC_WATERSENSOR_TASK_STACK 4096
#define ZIC_OTA_TASK_STACK 16384
#define ZIC_CTRL_TASK_PRIO 8
#define ZIC_TELEM_TASK_PRIO 5
#define ZIC_WATERSENSOR_TASK_PRIO 9

#define ZIC_EVENT_ENGINE_READY BIT0
#define ZIC_EVENT_FAULT BIT1
#define ZIC_EVENT_OTA_CONFIRMED BIT2

#define ZIC_DEFAULT_BROKER_URI "mqtt://192.168.10.2:1883"

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
    valve_diagnostics_t valve_diagnostics;
    storage_manager_t storage_manager;
    weather_snapshot_t weather_snapshot;
    weather_manager_t weather_manager;
    weather_decision_t weather_decision;
    et_output_t et_output;
    char state_topic[ZIC_MQTT_TOPIC_MAX];
    mqtt_transport_t mqtt_transport;
    bool scheduled_program_active;
    uint8_t scheduled_program_index;
    uint8_t scheduled_next_zone_index;
    config_program_t scheduled_program;
    uint32_t last_program_trigger_minute[CONFIG_MAX_PROGRAMS];
    flow_supervision_config_t flow_supervision;
    pressure_supervision_config_t pressure_supervision;
    config_hydraulic_t hydraulic_config;
    bool pressure_available;
    bool safety_fault_latched;
    uint32_t last_log_maintenance_timestamp;
    uint32_t last_weather_calculation_timestamp;
} zic_app_context_t;

static zic_app_context_t s_ctx;
static QueueHandle_t s_command_queue;
static EventGroupHandle_t s_runtime_events;
static SemaphoreHandle_t s_ctx_lock;
static SemaphoreHandle_t s_watersensor_lock;
static watersensor_client_t s_watersensor;

static const char *s_command_topics[] = {
    ZIC_MQTT_ROOT "/command/start_zone",
    ZIC_MQTT_ROOT "/command/stop_zone",
    ZIC_MQTT_ROOT "/command/run_program",
    ZIC_MQTT_ROOT "/command/stop_all",
    ZIC_MQTT_ROOT "/command/rain_delay",
    ZIC_MQTT_ROOT "/command/ota",
    "zmartify/v2/devices/" ZIC_V2_DEVICE_ID "/commands/irrigation/#",
};

static char s_v2_state_topic[128];
static char s_v2_outcome_topic[128];
static char s_v2_last_command_id[40];
static httpd_handle_t s_ota_http_server;

static uint32_t zic_log_timestamp(void)
{
    uint32_t epoch = 0;
    return hal_time_is_synced() && hal_time_get_epoch(&epoch) == HAL_OK ? epoch : 0;
}

static void zic_ota_audit(void *context, const char *message)
{
    zic_app_context_t *ctx = context;
    if (ctx == NULL || message == NULL || s_ctx_lock == NULL ||
        xSemaphoreTake(s_ctx_lock, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return;
    }

    storage_manager_append(&ctx->storage_manager, zic_log_timestamp(), ZIC_LOG_AUDIT, message);
    (void)storage_manager_flush(&ctx->storage_manager, zic_log_timestamp());
    xSemaphoreGive(s_ctx_lock);
}

static bool zic_diagnostics_snapshot(void *context,
                                     uint8_t *active_alarms,
                                     uint8_t *critical_alarms,
                                     uint32_t *log_entries,
                                     bool *subsystems_ready)
{
    zic_app_context_t *ctx = context;
    if (ctx == NULL || active_alarms == NULL || critical_alarms == NULL ||
        log_entries == NULL || subsystems_ready == NULL || s_ctx_lock == NULL ||
        xSemaphoreTake(s_ctx_lock, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return false;
    }

    *active_alarms = 0;
    *critical_alarms = 0;
    for (size_t index = 0; index < ZIC_MAX_ACTIVE_ALARMS; ++index) {
        const zic_alarm_t *alarm = &ctx->alarm_manager.alarms[index];
        if (alarm->active) {
            ++*active_alarms;
            if (alarm->severity == ZIC_ALARM_CRITICAL) {
                ++*critical_alarms;
            }
        }
    }
    *log_entries = (uint32_t)storage_manager_count(&ctx->storage_manager);
    EventBits_t runtime_bits = xEventGroupGetBits(s_runtime_events);
    *subsystems_ready = (runtime_bits & ZIC_EVENT_ENGINE_READY) != 0 &&
        (runtime_bits & ZIC_EVENT_FAULT) == 0;
#if ZIC_OTA_FORCE_HEALTH_FAILURE
    *subsystems_ready = false;
#endif
    xSemaphoreGive(s_ctx_lock);
    return true;
}

static void zic_diagnostics_raise_critical(void *context)
{
    zic_app_context_t *ctx = context;
    if (ctx == NULL || s_ctx_lock == NULL ||
        xSemaphoreTake(s_ctx_lock, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return;
    }
    alarm_manager_raise(&ctx->alarm_manager,
                        ZIC_ALARM_IRRIGATION_FAULT,
                        ZIC_ALARM_CRITICAL);
    xSemaphoreGive(s_ctx_lock);
}

static void zic_diagnostics_ota_confirmed(void *context)
{
    (void)context;
    if (s_runtime_events != NULL) {
        xEventGroupSetBits(s_runtime_events, ZIC_EVENT_OTA_CONFIRMED);
    }
}

static bool zic_log_load(void *context, void *data, size_t *length)
{
    (void)context;
    return hal_storage_read_blob("event_log", data, length) == HAL_OK;
}

static bool zic_log_save(void *context, const void *data, size_t length)
{
    (void)context;
    return hal_storage_write_blob("event_log", data, length) == HAL_OK;
}

static bool zic_weather_load(void *context, void *data, size_t *length)
{
    (void)context;
    return hal_storage_read_blob("weather", data, length) == HAL_OK;
}

static bool zic_weather_save(void *context, const void *data, size_t length)
{
    (void)context;
    return hal_storage_write_blob("weather", data, length) == HAL_OK;
}

static void zic_ota_restart_task(void *arg)
{
    (void)arg;
    vTaskDelay(pdMS_TO_TICKS(500));
    esp_restart();
}

static bool zic_ota_runtime_is_idle(void)
{
    if (s_ctx_lock == NULL || xSemaphoreTake(s_ctx_lock, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return false;
    }
    bool idle = irrigation_engine_is_idle(&s_ctx.engine) && !s_ctx.scheduled_program_active;
    xSemaphoreGive(s_ctx_lock);
    return idle;
}

static void zic_remote_ota_task(void *arg)
{
    char *firmware_url = arg;
    ota_manager_config_t ota_cfg = {
        .firmware_url = firmware_url,
        .cert_pem = NULL,
    };

    (void)event_bus_publish(EVENT_OTA_STARTED, 0, EVENT_PRIORITY_HIGH, 0, NULL, 0);
    zic_ota_audit(&s_ctx, "ota remote download started");
    if (ota_manager_perform(&ota_cfg)) {
        zic_ota_audit(&s_ctx, "ota signed image queued; health confirmation pending");
        free(firmware_url);
        zic_ota_restart_task(NULL);
        return;
    }

    zic_ota_audit(&s_ctx, "ota remote update rejected");
    (void)event_bus_publish(EVENT_OTA_FAILED, 0, EVENT_PRIORITY_HIGH, 0, NULL, 0);
    free(firmware_url);
    vTaskDelete(NULL);
}

static esp_err_t zic_ota_http_handler(httpd_req_t *request)
{
    const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
    if (partition == NULL || request->content_len <= 0 ||
        (size_t)request->content_len > partition->size) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Invalid firmware size");
        return ESP_FAIL;
    }
    if (!zic_ota_runtime_is_idle()) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Irrigation must be idle for OTA");
        return ESP_FAIL;
    }
    if (!ota_manager_transition(OTA_STATE_DOWNLOADING)) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "OTA already in progress");
        return ESP_FAIL;
    }
    zic_ota_audit(&s_ctx, "ota direct upload started");

    esp_ota_handle_t ota_handle = 0;
    esp_err_t err = esp_ota_begin(partition, request->content_len, &ota_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Direct OTA begin failed: %s", esp_err_to_name(err));
        (void)ota_manager_transition(OTA_STATE_FAILED);
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA begin failed");
        return ESP_FAIL;
    }

    char buffer[2048];
    int remaining = request->content_len;
    while (remaining > 0) {
        int received = httpd_req_recv(request, buffer,
                                      remaining < (int)sizeof(buffer) ? remaining : (int)sizeof(buffer));
        if (received == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        }
        if (received <= 0) {
            ESP_LOGE(TAG, "Direct OTA upload interrupted");
            esp_ota_abort(ota_handle);
            (void)ota_manager_transition(OTA_STATE_FAILED);
            zic_ota_audit(&s_ctx, "ota direct upload interrupted");
            return ESP_FAIL;
        }

        err = esp_ota_write(ota_handle, buffer, received);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Direct OTA write failed: %s", esp_err_to_name(err));
            esp_ota_abort(ota_handle);
            (void)ota_manager_transition(OTA_STATE_FAILED);
            zic_ota_audit(&s_ctx, "ota direct upload write failed");
            httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "OTA write failed");
            return ESP_FAIL;
        }
        remaining -= received;
    }

    (void)ota_manager_transition(OTA_STATE_VERIFYING);
    err = esp_ota_end(ota_handle);
    if (err == ESP_OK) {
        err = esp_ota_set_boot_partition(partition);
    }
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Direct OTA image validation failed: %s", esp_err_to_name(err));
        (void)ota_manager_transition(OTA_STATE_FAILED);
        zic_ota_audit(&s_ctx, "ota image signature or integrity rejected");
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Invalid firmware image");
        return ESP_FAIL;
    }

    (void)ota_manager_transition(OTA_STATE_PENDING_REBOOT);
    zic_ota_audit(&s_ctx, "ota signed image queued; health confirmation pending");
    ESP_LOGI(TAG, "Direct OTA complete; rebooting into %s", partition->label);
    httpd_resp_set_type(request, "text/plain");
    httpd_resp_sendstr(request, "OTA complete; rebooting\n");
    xTaskCreate(zic_ota_restart_task, "ota_restart", 2048, NULL, 5, NULL);
    return ESP_OK;
}

static esp_err_t zic_logs_http_handler(httpd_req_t *request)
{
    if (s_ctx_lock == NULL) {
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Runtime not ready");
        return ESP_FAIL;
    }

    char query[32] = {0};
    char format[8] = "json";
    size_t query_length = httpd_req_get_url_query_len(request);
    if (query_length > 0 && query_length < sizeof(query) &&
        httpd_req_get_url_query_str(request, query, sizeof(query)) == ESP_OK) {
        (void)httpd_query_key_value(query, "format", format, sizeof(format));
    }
    bool csv = strcmp(format, "csv") == 0;
    httpd_resp_set_type(request, csv ? "text/csv" : "application/json");

    if (xSemaphoreTake(s_ctx_lock, pdMS_TO_TICKS(1000)) != pdTRUE) {
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Runtime busy");
        return ESP_FAIL;
    }

    esp_err_t result = ESP_OK;
    if (csv) {
        result = httpd_resp_send_chunk(request, "timestamp,type,message\n", 23);
    } else {
        result = httpd_resp_send_chunk(request, "[", 1);
    }

    char line[256];
    for (size_t index = 0; result == ESP_OK && index < storage_manager_count(&s_ctx.storage_manager);
         ++index) {
        zic_log_entry_t entry;
        if (!storage_manager_get(&s_ctx.storage_manager, index, &entry)) {
            continue;
        }
        size_t length = csv
            ? storage_manager_export_csv(&entry, line, sizeof(line))
            : storage_manager_export_json(&entry, line, sizeof(line));
        if (length == 0) {
            result = ESP_FAIL;
            break;
        }
        if (!csv && index > 0) {
            result = httpd_resp_send_chunk(request, ",", 1);
        }
        if (result == ESP_OK) {
            result = httpd_resp_send_chunk(request, line, length);
        }
        if (result == ESP_OK && csv) {
            result = httpd_resp_send_chunk(request, "\n", 1);
        }
    }
    if (result == ESP_OK && !csv) {
        result = httpd_resp_send_chunk(request, "]", 1);
    }
    if (result == ESP_OK) {
        result = httpd_resp_send_chunk(request, NULL, 0);
    }
    xSemaphoreGive(s_ctx_lock);
    return result;
}

static bool zic_json_copy_string(const cJSON *root, const char *key, char *out, size_t out_len)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (!cJSON_IsString(item) || item->valuestring == NULL) {
        return true;
    }
    size_t len = strlen(item->valuestring);
    if (len >= out_len) {
        return false;
    }
    memcpy(out, item->valuestring, len + 1U);
    return true;
}

static esp_err_t zic_network_config_http_handler(httpd_req_t *request)
{
    char body[512];
    if (request->content_len <= 0 || request->content_len >= (int)sizeof(body)) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Invalid config payload size");
        return ESP_FAIL;
    }

    int received_total = 0;
    while (received_total < request->content_len) {
        int received = httpd_req_recv(request,
                                      body + received_total,
                                      request->content_len - received_total);
        if (received == HTTPD_SOCK_ERR_TIMEOUT) {
            continue;
        }
        if (received <= 0) {
            httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Invalid config payload");
            return ESP_FAIL;
        }
        received_total += received;
    }
    body[received_total] = '\0';

    cJSON *json = cJSON_Parse(body);
    if (json == NULL) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }

    config_network_t network;
    if (config_get_network(&network) != CFG_OK) {
        cJSON_Delete(json);
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Config not ready");
        return ESP_FAIL;
    }

    bool ok = true;
    ok = ok && zic_json_copy_string(json, "mqtt_broker_uri", network.mqtt_broker_uri, sizeof(network.mqtt_broker_uri));
    ok = ok && zic_json_copy_string(json, "mqtt_uri", network.mqtt_broker_uri, sizeof(network.mqtt_broker_uri));
    ok = ok && zic_json_copy_string(json, "mqtt_username", network.mqtt_username, sizeof(network.mqtt_username));
    ok = ok && zic_json_copy_string(json, "mqtt_password", network.mqtt_password, sizeof(network.mqtt_password));
    ok = ok && zic_json_copy_string(json, "ntp_server", network.ntp_server, sizeof(network.ntp_server));
    ok = ok && zic_json_copy_string(json, "timezone", network.timezone, sizeof(network.timezone));

    const cJSON *tls = cJSON_GetObjectItemCaseSensitive(json, "mqtt_tls_enabled");
    if (cJSON_IsBool(tls)) {
        network.mqtt_tls_enabled = cJSON_IsTrue(tls);
    }
    const cJSON *port = cJSON_GetObjectItemCaseSensitive(json, "mqtt_port");
    if (cJSON_IsNumber(port) && port->valuedouble >= 0 && port->valuedouble <= 65535) {
        network.mqtt_port = (uint16_t)port->valuedouble;
    }

    cJSON_Delete(json);
    if (!ok) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Config value too long");
        return ESP_FAIL;
    }
    if (config_set_network(&network) != CFG_OK || config_manager_commit() != CFG_OK) {
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Config save failed");
        return ESP_FAIL;
    }

    httpd_resp_set_type(request, "application/json");
    httpd_resp_sendstr(request, "{\"ok\":true,\"reboot_required\":true}");
    return ESP_OK;
}

static bool zic_json_float(const cJSON *json, const char *key, float *value, bool required)
{
    const cJSON *item = cJSON_GetObjectItemCaseSensitive(json, key);
    if (item == NULL) {
        return !required;
    }
    if (!cJSON_IsNumber(item)) {
        return false;
    }
    *value = (float)item->valuedouble;
    return true;
}

static esp_err_t zic_weather_http_handler(httpd_req_t *request)
{
    if (request->content_len <= 0 || request->content_len >= 512 || s_ctx_lock == NULL) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Invalid weather payload");
        return ESP_FAIL;
    }
    char body[512];
    int received_total = 0;
    while (received_total < request->content_len) {
        int received = httpd_req_recv(request, body + received_total,
                                      request->content_len - received_total);
        if (received <= 0) {
            httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Incomplete weather payload");
            return ESP_FAIL;
        }
        received_total += received;
    }
    body[received_total] = '\0';

    cJSON *json = cJSON_Parse(body);
    weather_snapshot_t snapshot = {0};
    const cJSON *timestamp = json != NULL
        ? cJSON_GetObjectItemCaseSensitive(json, "timestamp")
        : NULL;
    if (timestamp != NULL && cJSON_IsNumber(timestamp)) {
        snapshot.timestamp = (uint32_t)timestamp->valuedouble;
    } else {
        snapshot.timestamp = zic_log_timestamp();
    }
    bool valid = json != NULL && snapshot.timestamp != 0 &&
        zic_json_float(json, "temperature_c", &snapshot.temperature_c, true) &&
        zic_json_float(json, "humidity_pct", &snapshot.humidity_pct, true) &&
        zic_json_float(json, "wind_speed_mps", &snapshot.wind_speed_mps, true) &&
        zic_json_float(json, "solar_radiation_mj_m2", &snapshot.solar_radiation_mj_m2, true) &&
        zic_json_float(json, "rain_mm_last_24h", &snapshot.rain_mm_last_24h, true) &&
        zic_json_float(json, "rain_probability_pct", &snapshot.rain_probability_pct, true) &&
        zic_json_float(json, "uv_index", &snapshot.uv_index, false);
    snapshot.valid = valid;

    bool updated = false;
    if (valid && xSemaphoreTake(s_ctx_lock, pdMS_TO_TICKS(1000)) == pdTRUE) {
        updated = weather_manager_update(&s_ctx.weather_manager, &snapshot);
        if (updated) {
            storage_manager_append(&s_ctx.storage_manager, snapshot.timestamp,
                                   ZIC_LOG_WEATHER, "weather snapshot updated");
        }
        xSemaphoreGive(s_ctx_lock);
    }
    cJSON_Delete(json);
    if (!updated) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Weather snapshot rejected");
        return ESP_FAIL;
    }
    httpd_resp_set_type(request, "application/json");
    httpd_resp_sendstr(request, "{\"status\":\"accepted\"}");
    return ESP_OK;
}

static bool zic_ota_http_start(void)
{
    if (s_ota_http_server != NULL) {
        return true;
    }

    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = ZIC_OTA_TASK_STACK;
    esp_err_t err = httpd_start(&s_ota_http_server, &config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Direct OTA HTTP server failed: %s", esp_err_to_name(err));
        return false;
    }

    const httpd_uri_t ota_uri = {
        .uri = "/ota",
        .method = HTTP_POST,
        .handler = zic_ota_http_handler,
    };
    err = httpd_register_uri_handler(s_ota_http_server, &ota_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Direct OTA endpoint registration failed: %s", esp_err_to_name(err));
        httpd_stop(s_ota_http_server);
        s_ota_http_server = NULL;
        return false;
    }

    const httpd_uri_t logs_uri = {
        .uri = "/logs",
        .method = HTTP_GET,
        .handler = zic_logs_http_handler,
    };
    err = httpd_register_uri_handler(s_ota_http_server, &logs_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Log export endpoint registration failed: %s", esp_err_to_name(err));
        httpd_stop(s_ota_http_server);
        s_ota_http_server = NULL;
        return false;
    }

    const httpd_uri_t network_config_uri = {
        .uri = "/config/network",
        .method = HTTP_POST,
        .handler = zic_network_config_http_handler,
    };
    err = httpd_register_uri_handler(s_ota_http_server, &network_config_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Network config endpoint registration failed: %s", esp_err_to_name(err));
        httpd_stop(s_ota_http_server);
        s_ota_http_server = NULL;
        return false;
    }

    const httpd_uri_t weather_uri = {
        .uri = "/weather",
        .method = HTTP_POST,
        .handler = zic_weather_http_handler,
    };
    err = httpd_register_uri_handler(s_ota_http_server, &weather_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Weather endpoint registration failed: %s", esp_err_to_name(err));
        httpd_stop(s_ota_http_server);
        s_ota_http_server = NULL;
        return false;
    }

    ESP_LOGI(TAG, "HTTP services ready: POST /ota, GET /logs, POST /config/network, POST /weather");
    return true;
}

static void zic_wifi_event_handler(void *arg,
                                   esp_event_base_t event_base,
                                   int32_t event_id,
                                   void *event_data)
{
    (void)arg;

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "Wi-Fi connecting");
        (void)esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        const wifi_event_sta_disconnected_t *event =
            (const wifi_event_sta_disconnected_t *)event_data;
        ESP_LOGW(TAG, "Wi-Fi disconnected (reason=%u); reconnecting", event->reason);
        (void)esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *event = (const ip_event_got_ip_t *)event_data;
        ESP_LOGI(TAG, "Wi-Fi connected, IP=" IPSTR, IP2STR(&event->ip_info.ip));
        (void)zic_ota_http_start();
    }
}

static bool zic_wifi_init(void)
{
    if (zic_wifi_ssid[0] == 0) {
        ESP_LOGW(TAG, "Wi-Fi credentials not provisioned; run scripts/configure-wifi.sh");
        return false;
    }

    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(err));
        return false;
    }

    if (esp_netif_create_default_wifi_sta() == NULL) {
        ESP_LOGE(TAG, "Failed to create Wi-Fi STA interface");
        return false;
    }

    wifi_init_config_t init_config = WIFI_INIT_CONFIG_DEFAULT();
    if (esp_wifi_init(&init_config) != ESP_OK) {
        ESP_LOGE(TAG, "esp_wifi_init failed");
        return false;
    }

    (void)esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, zic_wifi_event_handler, NULL);
    (void)esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, zic_wifi_event_handler, NULL);

    wifi_config_t wifi_config = {0};
    memcpy(wifi_config.sta.ssid, zic_wifi_ssid, sizeof(zic_wifi_ssid) - 1U);
    memcpy(wifi_config.sta.password, zic_wifi_password, sizeof(zic_wifi_password) - 1U);
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wifi_config.sta.pmf_cfg.capable = true;
    wifi_config.sta.pmf_cfg.required = false;

    if (esp_wifi_set_mode(WIFI_MODE_STA) != ESP_OK ||
        esp_wifi_set_config(WIFI_IF_STA, &wifi_config) != ESP_OK ||
        esp_wifi_start() != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start Wi-Fi STA");
        return false;
    }

    ESP_LOGI(TAG, "Wi-Fi STA started for SSID '%s'", (const char *)zic_wifi_ssid);
    return true;
}

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

static bool zic_uri_uses_tls(const char *uri)
{
    return uri != NULL && strncmp(uri, "mqtts://", strlen("mqtts://")) == 0;
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

    if (strcmp(topic_buf, ZIC_MQTT_ROOT "/command/ota") == 0) {
        json = zic_parse_json_payload(payload, payload_len, json_scratch, sizeof(json_scratch));
        if (json == NULL) {
            ESP_LOGW(TAG, "Invalid OTA command payload");
            return;
        }

        const cJSON *url = cJSON_GetObjectItemCaseSensitive(json, "url");
        if (cJSON_IsString(url) && url->valuestring != NULL && url->valuestring[0] != '\0') {
            cmd.type = ZIC_CMD_TRIGGER_OTA;
            strncpy(cmd.ota_url, url->valuestring, sizeof(cmd.ota_url) - 1U);
            cmd.ota_url[sizeof(cmd.ota_url) - 1U] = '\0';
            (void)xQueueSend(s_command_queue, &cmd, 0);
        } else {
            ESP_LOGW(TAG, "OTA command requires a non-empty url");
        }
        cJSON_Delete(json);
        return;
    }

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
    persistent_store_init();

    irrigation_engine_init(&ctx->engine);
    alarm_manager_init(&ctx->alarm_manager);
    flow_manager_init(&ctx->flow_manager);
    config_alarms_t alarm_config;
    (void)config_get_alarms(&alarm_config);
    (void)config_get_hydraulics(&ctx->hydraulic_config);
    pressure_manager_init(&ctx->pressure_manager,
                          alarm_config.pressure_low_mbar,
                          alarm_config.pressure_high_mbar);
    ctx->pressure_supervision.low_pressure_mbar = alarm_config.pressure_low_mbar;
    ctx->pressure_supervision.high_pressure_mbar = alarm_config.pressure_high_mbar;
    ctx->pressure_supervision.critical_duration_ms =
        (uint32_t)alarm_config.pressure_critical_duration_s * 1000u;
    ctx->flow_supervision.low_flow_lpm_x100 = (uint32_t)alarm_config.flow_low_lpm_x10 * 10u;
    ctx->flow_supervision.high_flow_lpm_x100 = (uint32_t)alarm_config.flow_high_lpm_x10 * 10u;
    ctx->flow_supervision.no_flow_timeout_ms = alarm_config.no_flow_timeout_s * 1000u;
    ctx->flow_supervision.high_flow_duration_ms = alarm_config.high_flow_duration_s * 1000u;
    ctx->flow_supervision.active_max_age_ms = alarm_config.flow_active_max_age_ms;
    ctx->flow_supervision.idle_max_age_ms = alarm_config.flow_idle_max_age_ms;
    valve_diag_config_t valve_diag_config = {
        .open_response_timeout_ms = ctx->hydraulic_config.valve_open_timeout_s * 1000u,
        .close_response_timeout_ms = ctx->hydraulic_config.valve_close_timeout_s * 1000u,
        .minimum_flow_lpm_x100 = (uint32_t)alarm_config.flow_low_lpm_x10 * 10u,
        .minimum_pressure_mbar = alarm_config.pressure_low_mbar,
    };
    (void)valve_diagnostics_init(&ctx->valve_diagnostics, &valve_diag_config);
    storage_manager_init(&ctx->storage_manager, s_log_buffer, ZIC_LOG_PERSIST_CAPACITY);
    storage_manager_set_persistence(&ctx->storage_manager, zic_log_load, zic_log_save, NULL);
    if (!storage_manager_restore(&ctx->storage_manager)) {
        ESP_LOGI(TAG, "No valid persisted event log found");
    }
    storage_manager_append(&ctx->storage_manager, zic_log_timestamp(), ZIC_LOG_AUDIT,
                           "controller boot");
    weather_manager_init(&ctx->weather_manager);
    weather_manager_set_persistence(&ctx->weather_manager,
                                    zic_weather_load, zic_weather_save, NULL);
    if (!weather_manager_restore(&ctx->weather_manager)) {
        ESP_LOGI(TAG, "No valid cached weather snapshot found");
    }
    zic_mqtt_topic_build("state", ctx->state_topic, sizeof(ctx->state_topic));
    zic_v2_state_topic(ZIC_V2_DEVICE_ID, s_v2_state_topic, sizeof(s_v2_state_topic));
    zic_v2_outcome_topic(ZIC_V2_DEVICE_ID, s_v2_outcome_topic, sizeof(s_v2_outcome_topic));

    config_network_t network_config = {0};
    const bool has_network_config = config_get_network(&network_config) == CFG_OK;
    const char *broker_uri = (has_network_config && network_config.mqtt_broker_uri[0] != '\0')
        ? network_config.mqtt_broker_uri
        : ZIC_DEFAULT_BROKER_URI;
    const char *username = (has_network_config && network_config.mqtt_username[0] != '\0')
        ? network_config.mqtt_username
        : NULL;
    const char *password = (has_network_config && network_config.mqtt_password[0] != '\0')
        ? network_config.mqtt_password
        : NULL;

    mqtt_transport_config_t mqtt_cfg = {
        .broker_uri = broker_uri,
        .client_id = ZIC_V2_DEVICE_ID,
        .username = username,
        .password = password,
        .use_crt_bundle = (has_network_config && network_config.mqtt_tls_enabled) || zic_uri_uses_tls(broker_uri),
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

static bool zic_runtime_start_zone(zic_app_context_t *ctx,
                                   uint8_t zone_id,
                                   uint32_t requested_runtime_seconds)
{
    ota_state_t ota_state = ota_manager_get_state();
    if ((xEventGroupGetBits(s_runtime_events) & ZIC_EVENT_OTA_CONFIRMED) == 0 ||
        (ota_state != OTA_STATE_IDLE && ota_state != OTA_STATE_VALID &&
         ota_state != OTA_STATE_FAILED)) {
        return false;
    }

    config_zone_t zone;
    config_system_t system;
    if (zone_id == 0 || config_get_zone(zone_id - 1, &zone) != CFG_OK ||
        config_get_system(&system) != CFG_OK || !zone.enabled ||
        system.operational_mode == CONFIG_MODE_OFF) {
        return false;
    }

    uint32_t runtime_seconds = requested_runtime_seconds;
    if (runtime_seconds == 0) {
        runtime_seconds = zone.default_runtime_s;
    }
    if (runtime_seconds > zone.max_runtime_s) {
        runtime_seconds = zone.max_runtime_s;
    }
    if (runtime_seconds > system.global_max_runtime_s) {
        runtime_seconds = system.global_max_runtime_s;
    }

    uint32_t baseline_lpm_x100 = (uint32_t)zone.flow_baseline_lpm_x10 * 10u;
    uint32_t critical_delta =
        (baseline_lpm_x100 * zone.flow_critical_deviation_pct) / 100u;
    uint32_t zone_low_flow = baseline_lpm_x100 - critical_delta;
    uint32_t zone_high_flow = baseline_lpm_x100 + critical_delta;
    config_alarms_t alarms;
    if (config_get_alarms(&alarms) == CFG_OK) {
        uint32_t absolute_low = (uint32_t)alarms.flow_low_lpm_x10 * 10u;
        uint32_t absolute_high = (uint32_t)alarms.flow_high_lpm_x10 * 10u;
        ctx->flow_supervision.low_flow_lpm_x100 =
            zone_low_flow > absolute_low ? zone_low_flow : absolute_low;
        ctx->flow_supervision.high_flow_lpm_x100 =
            zone_high_flow < absolute_high ? zone_high_flow : absolute_high;
    }
    flow_manager_set_baseline(&ctx->flow_manager, baseline_lpm_x100);
    flow_manager_set_deviation_limits(&ctx->flow_manager,
                                      zone.flow_warning_deviation_pct,
                                      zone.flow_critical_deviation_pct);
    ctx->pressure_supervision.low_pressure_mbar = zone.pressure_min_mbar;
    ctx->pressure_supervision.high_pressure_mbar = zone.pressure_max_mbar;
    if (!irrigation_engine_start_zone(&ctx->engine, zone_id, zone.relay_index,
                                      runtime_seconds,
                                      (uint64_t)(esp_timer_get_time() / 1000))) {
        return false;
    }
    storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                           ZIC_LOG_IRRIGATION, "zone started");
    return true;
}

static void zic_runtime_apply_command(zic_app_context_t *ctx, const zic_runtime_command_t *cmd)
{
    switch (cmd->type) {
    case ZIC_CMD_START_ZONE: {
        if (zic_runtime_start_zone(ctx, cmd->zone_id, cmd->runtime_seconds)) {
            zic_publish_v2_outcome(ctx, "run.started", "info", "accepted", NULL, cmd->zone_id);
        } else {
            zic_publish_v2_outcome(ctx, "run.rejected", "warning", "rejected",
                                   "controller_not_idle", cmd->zone_id);
        }
        break;
    }
    case ZIC_CMD_STOP_ZONE:
        ctx->scheduled_program_active = false;
        if (irrigation_engine_stop_zone(&ctx->engine, cmd->zone_id)) {
            valve_diagnostics_command_close(&ctx->valve_diagnostics,
                                            (uint64_t)(esp_timer_get_time() / 1000));
            storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                           ZIC_LOG_IRRIGATION, "zone stopped by command");
            zic_publish_v2_outcome(ctx, "run.stopped", "info", "completed", NULL, cmd->zone_id);
        } else {
            zic_publish_v2_outcome(ctx, "run.stop_rejected", "warning", "rejected",
                                   "zone_not_active", cmd->zone_id);
        }
        break;
    case ZIC_CMD_STOP_ALL:
        ctx->scheduled_program_active = false;
        if (!irrigation_engine_is_idle(&ctx->engine)) {
            valve_diagnostics_command_close(&ctx->valve_diagnostics,
                                            (uint64_t)(esp_timer_get_time() / 1000));
        }
        irrigation_engine_stop_all(&ctx->engine);
        storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                       ZIC_LOG_AUDIT, "all irrigation stopped by command");
        zic_publish_v2_outcome(ctx, "run.stopped", "info", "completed", "stop_all", 0);
        break;
    case ZIC_CMD_SET_RAIN_DELAY:
        ctx->scheduled_program_active = false;
        if (!irrigation_engine_is_idle(&ctx->engine)) {
            valve_diagnostics_command_close(&ctx->valve_diagnostics,
                                            (uint64_t)(esp_timer_get_time() / 1000));
        }
        (void)irrigation_engine_stop_all(&ctx->engine);
        persistent_store_set_u32("rain_delay_h", cmd->rain_delay_hours);
        zic_controller_apply_event(&ctx->engine.controller, ZIC_EV_RAIN_DELAY_SET, -1);
        storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                       ZIC_LOG_AUDIT, "rain delay set");
        zic_publish_v2_outcome(ctx, "rain.delay_set", "info", "accepted", NULL, 0);
        break;
    case ZIC_CMD_CLEAR_RAIN_DELAY:
        persistent_store_set_u32("rain_delay_h", 0);
        zic_controller_apply_event(&ctx->engine.controller, ZIC_EV_RAIN_DELAY_CLEAR, -1);
        storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                       ZIC_LOG_AUDIT, "rain delay cleared");
        zic_publish_v2_outcome(ctx, "rain.delay_cleared", "info", "accepted", NULL, 0);
        break;
    case ZIC_CMD_TRIGGER_OTA: {
        if (cmd->ota_url[0] == '\0' || !irrigation_engine_is_idle(&ctx->engine) ||
            ctx->scheduled_program_active) {
            storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                                   ZIC_LOG_AUDIT, "ota rejected; irrigation not idle");
            break;
        }

        char *firmware_url = malloc(strlen(cmd->ota_url) + 1U);
        if (firmware_url == NULL) {
            storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                                   ZIC_LOG_AUDIT, "ota rejected; task allocation failed");
            break;
        }
        strcpy(firmware_url, cmd->ota_url);
        if (xTaskCreate(zic_remote_ota_task, "ota_remote", ZIC_OTA_TASK_STACK,
                firmware_url, 4, NULL) != pdPASS) {
            free(firmware_url);
            storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                                   ZIC_LOG_AUDIT, "ota rejected; task creation failed");
        }
        break;
    }
    default:
        break;
    }
}

static void zic_scheduler_start_due_program(zic_app_context_t *ctx)
{
    if ((xEventGroupGetBits(s_runtime_events) & ZIC_EVENT_OTA_CONFIRMED) == 0 ||
        !hal_time_is_synced() || !irrigation_engine_is_idle(&ctx->engine) ||
        ctx->scheduled_program_active) {
        return;
    }

    config_system_t system;
    struct tm local_time;
    uint32_t epoch;
    if (config_get_system(&system) != CFG_OK || system.operational_mode != CONFIG_MODE_AUTO ||
        hal_time_get_local(&local_time) != HAL_OK || hal_time_get_epoch(&epoch) != HAL_OK) {
        return;
    }

    uint32_t minute_key = epoch / 60u;
    config_days_t today = (config_days_t)(1u << local_time.tm_wday);
    for (uint8_t program_index = 0; program_index < CONFIG_MAX_PROGRAMS; ++program_index) {
        config_program_t program;
        if (config_get_program(program_index, &program) != CFG_OK || !program.enabled ||
            (program.run_days & today) == 0 ||
            ctx->last_program_trigger_minute[program_index] == minute_key) {
            continue;
        }

        bool due = false;
        for (uint8_t start_index = 0; start_index < CONFIG_MAX_START_TIMES; ++start_index) {
            const config_start_time_t *start = &program.start_times[start_index];
            if (start->enabled && start->hour == local_time.tm_hour &&
                start->minute == local_time.tm_min) {
                due = true;
                break;
            }
        }
        if (!due) {
            continue;
        }

        ctx->last_program_trigger_minute[program_index] = minute_key;
        weather_snapshot_t weather;
        bool weather_available = weather_manager_get_snapshot(&ctx->weather_manager, epoch, &weather);
        if (program.weather_skip_enabled && weather_available &&
            weather.rain_probability_pct >= program.rain_skip_threshold_pct) {
            ESP_LOGI(TAG, "Program %u skipped due to rain probability", program_index + 1);
            continue;
        }

        ctx->scheduled_program_active = true;
        ctx->scheduled_program_index = program_index;
        ctx->scheduled_next_zone_index = 0;
        ctx->scheduled_program = program;
        storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                       ZIC_LOG_IRRIGATION, "scheduled program started");
        ESP_LOGI(TAG, "Scheduled program %u started", program_index + 1);
        return;
    }
}

static void zic_scheduler_advance_program(zic_app_context_t *ctx)
{
    if (!ctx->scheduled_program_active || !irrigation_engine_is_idle(&ctx->engine)) {
        return;
    }

    config_system_t system;
    if (config_get_system(&system) != CFG_OK || system.operational_mode != CONFIG_MODE_AUTO) {
        ctx->scheduled_program_active = false;
        return;
    }

    while (ctx->scheduled_next_zone_index < CONFIG_MAX_ZONES) {
        uint8_t zone_index = ctx->scheduled_next_zone_index++;
        config_zone_t zone;
        if (config_get_zone(zone_index, &zone) != CFG_OK || !zone.enabled) {
            continue;
        }
        uint32_t base_runtime_seconds =
            (uint32_t)ctx->scheduled_program.zone_runtime_min[zone_index] * 60u;
        uint16_t seasonal_percent =
            (uint16_t)ctx->scheduled_program.seasonal_adjust_pct * zone.seasonal_factor_pct / 100u;
        uint32_t epoch = 0;
        weather_snapshot_t weather;
        const weather_snapshot_t *weather_ptr = NULL;
        if (hal_time_get_epoch(&epoch) == HAL_OK &&
            weather_manager_get_snapshot(&ctx->weather_manager, epoch, &weather)) {
            weather_ptr = &weather;
        }
        weather_adjustment_t adjustment;
        (void)weather_manager_adjust_runtime(weather_ptr, epoch, base_runtime_seconds,
                                             ctx->et_output.daily_et_mm,
                                             zone.et_crop_coefficient_x100,
                                             seasonal_percent, &adjustment);
        uint32_t runtime_seconds = adjustment.runtime_seconds;
        if (adjustment.skip_watering) {
            storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                                   ZIC_LOG_WEATHER, "zone skipped by weather adjustment");
        }
        if (runtime_seconds != 0 && zic_runtime_start_zone(ctx, zone_index + 1, runtime_seconds)) {
            ESP_LOGI(TAG, "Program %u running zone %u for %lu seconds",
                     ctx->scheduled_program_index + 1, zone_index + 1,
                     (unsigned long)runtime_seconds);
            return;
        }
    }

    ESP_LOGI(TAG, "Scheduled program %u completed", ctx->scheduled_program_index + 1);
    storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                           ZIC_LOG_IRRIGATION, "scheduled program completed");
    ctx->scheduled_program_active = false;
}

static void zic_control_task(void *arg)
{
    (void)arg;

    zic_runtime_command_t cmd;
    xEventGroupSetBits(s_runtime_events, ZIC_EVENT_ENGINE_READY);

    for (;;) {
        if (xQueueReceive(s_command_queue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE) {
            xSemaphoreTake(s_ctx_lock, portMAX_DELAY);
            zic_runtime_apply_command(&s_ctx, &cmd);
            xSemaphoreGive(s_ctx_lock);
        }

        xSemaphoreTake(s_ctx_lock, portMAX_DELAY);

        irrigation_phase_t previous_phase = s_ctx.engine.phase;
        uint64_t now_ms = (uint64_t)(esp_timer_get_time() / 1000);
        (void)irrigation_engine_tick(&s_ctx.engine, now_ms);
        if (previous_phase != IRRIGATION_PHASE_RUNNING &&
            s_ctx.engine.phase == IRRIGATION_PHASE_RUNNING) {
            valve_diagnostics_command_open(&s_ctx.valve_diagnostics,
                                           s_ctx.engine.active_zone_id, now_ms);
        } else if (previous_phase == IRRIGATION_PHASE_RUNNING &&
                   s_ctx.engine.phase != IRRIGATION_PHASE_RUNNING) {
            valve_diagnostics_command_close(&s_ctx.valve_diagnostics, now_ms);
        }
        if (previous_phase == IRRIGATION_PHASE_MASTER_CLOSE_DELAY &&
            irrigation_engine_is_idle(&s_ctx.engine)) {
            storage_manager_append(&s_ctx.storage_manager, zic_log_timestamp(),
                                   ZIC_LOG_IRRIGATION, "zone completed");
        }

        flow_measurement_t flow = {0};
        watersensor_snapshot_t snapshot;
        uint32_t snapshot_age_ms = 0;
        if (xSemaphoreTake(s_watersensor_lock, pdMS_TO_TICKS(50)) == pdTRUE) {
            if (watersensor_client_get_snapshot(&s_watersensor, &snapshot, &snapshot_age_ms)) {
                for (uint8_t index = 0; index < snapshot.flow_count; ++index) {
                    const watersensor_flow_record_t *record = &snapshot.flows[index];
                    if (record->channel_id == s_watersensor.config.flow_channel_id &&
                        (record->flags & WATERSENSOR_MEASUREMENT_VALID) != 0 &&
                        (record->flags & WATERSENSOR_MEASUREMENT_FAULT) == 0) {
                        flow.source = FLOW_SOURCE_WATERSENSOR;
                        flow.flow_lpm_x100 = record->flow_ml_min / 10U;
                        flow.measurement_age_ms = snapshot_age_ms;
                        flow.volume_total_ml = record->volume_total_ml;
                        flow.pulse_count_total = record->pulse_count_total;
                        flow.valid = true;
                        break;
                    }
                }
            }
            xSemaphoreGive(s_watersensor_lock);
        }
        bool irrigation_active = s_ctx.engine.phase == IRRIGATION_PHASE_RUNNING;
        (void)flow_manager_supervise(&s_ctx.flow_manager, &flow, &s_ctx.flow_supervision,
                                     irrigation_active,
                                     (uint64_t)(esp_timer_get_time() / 1000),
                                     &s_ctx.alarm_manager);

        bool pressure_valid = false;
        uint32_t pressure_mbar_value = 0;
        if (irrigation_active && s_ctx.pressure_available) {
            int32_t pressure_mv = 0;
            if (hal_pressure_read_voltage(HAL_ADC_CHANNEL_0, HAL_ADC_PGA_6V144,
                                          &pressure_mv) == HAL_OK &&
                s_ctx.hydraulic_config.pressure_mv_per_bar > 0.0f) {
                float pressure_mbar =
                    ((float)pressure_mv - s_ctx.hydraulic_config.pressure_offset_mv) * 1000.0f /
                    s_ctx.hydraulic_config.pressure_mv_per_bar;
                pressure_mbar_value = pressure_mbar > 0.0f ? (uint32_t)pressure_mbar : 0;
                pressure_valid = true;
            }
        }
        (void)pressure_manager_supervise(&s_ctx.pressure_manager,
                                         &s_ctx.pressure_supervision,
                                         pressure_valid,
                                         pressure_mbar_value,
                                         irrigation_active,
                                         (uint64_t)(esp_timer_get_time() / 1000),
                                         &s_ctx.alarm_manager);
        valve_diag_observation_t valve_observation = {
            .flow_valid = flow.valid &&
                flow.measurement_age_ms <= (irrigation_active
                    ? s_ctx.flow_supervision.active_max_age_ms
                    : s_ctx.flow_supervision.idle_max_age_ms),
            .flow_lpm_x100 = flow.flow_lpm_x100,
            .pressure_valid = pressure_valid,
            .pressure_mbar = pressure_mbar_value,
        };
        valve_diag_status_t previous_valve_status = s_ctx.valve_diagnostics.status;
        valve_diag_status_t valve_status = valve_diagnostics_evaluate(
            &s_ctx.valve_diagnostics, &valve_observation, now_ms);
        if (valve_status != previous_valve_status) {
            if (valve_status == VALVE_DIAG_SENSOR_UNAVAILABLE) {
                alarm_manager_raise(&s_ctx.alarm_manager,
                                    ZIC_ALARM_VALVE_DIAGNOSTICS_UNAVAILABLE,
                                    ZIC_ALARM_WARNING);
                storage_manager_append(&s_ctx.storage_manager, zic_log_timestamp(),
                                       ZIC_LOG_ALARM,
                                       "valve diagnostics unavailable; hydraulic sensors");
            } else {
                alarm_manager_clear(&s_ctx.alarm_manager,
                                    ZIC_ALARM_VALVE_DIAGNOSTICS_UNAVAILABLE);
            }
            if (valve_status == VALVE_DIAG_NO_RESPONSE) {
                alarm_manager_raise(&s_ctx.alarm_manager,
                                    ZIC_ALARM_VALVE_NO_RESPONSE,
                                    ZIC_ALARM_CRITICAL);
                storage_manager_append(&s_ctx.storage_manager, zic_log_timestamp(),
                                       ZIC_LOG_ALARM,
                                       "valve command produced no hydraulic response");
            } else if (valve_status == VALVE_DIAG_LIKELY_STUCK_CLOSED) {
                alarm_manager_raise(&s_ctx.alarm_manager,
                                    ZIC_ALARM_VALVE_LIKELY_STUCK_CLOSED,
                                    ZIC_ALARM_CRITICAL);
                storage_manager_append(&s_ctx.storage_manager, zic_log_timestamp(),
                                       ZIC_LOG_ALARM,
                                       "valve no response; likely stuck closed");
            } else if (valve_status == VALVE_DIAG_LIKELY_STUCK_OPEN) {
                alarm_manager_raise(&s_ctx.alarm_manager,
                                    ZIC_ALARM_VALVE_LIKELY_STUCK_OPEN,
                                    ZIC_ALARM_CRITICAL);
                storage_manager_append(&s_ctx.storage_manager, zic_log_timestamp(),
                                       ZIC_LOG_ALARM,
                                       "flow after close; likely stuck open");
            }
        }
        uint32_t weather_now = 0;
        weather_snapshot_t fresh_weather;
        if (hal_time_get_epoch(&weather_now) == HAL_OK &&
            weather_manager_get_snapshot(&s_ctx.weather_manager, weather_now, &fresh_weather)) {
            s_ctx.weather_snapshot = fresh_weather;
            weather_manager_evaluate(&fresh_weather, &s_ctx.weather_decision);
            if (fresh_weather.timestamp != s_ctx.last_weather_calculation_timestamp) {
                et_input_t et_input = {
                    .temperature_c = fresh_weather.temperature_c,
                    .humidity_pct = fresh_weather.humidity_pct,
                    .wind_speed_mps = fresh_weather.wind_speed_mps,
                    .solar_radiation_mj_m2 = fresh_weather.solar_radiation_mj_m2,
                };
                et_engine_compute(&et_input, &s_ctx.et_output);
                s_ctx.last_weather_calculation_timestamp = fresh_weather.timestamp;
            }
        } else {
            s_ctx.weather_snapshot = (weather_snapshot_t){0};
            s_ctx.weather_decision = (weather_decision_t){0};
            s_ctx.et_output = (et_output_t){0};
        }

        if (alarm_manager_has_severity(&s_ctx.alarm_manager, ZIC_ALARM_CRITICAL)) {
            if (!s_ctx.safety_fault_latched) {
                ESP_LOGE(TAG, "Critical hydraulic alarm; irrigation locked out until reboot");
                storage_manager_append(&s_ctx.storage_manager, zic_log_timestamp(), ZIC_LOG_ALARM,
                                       "critical hydraulic shutdown");
                (void)storage_manager_flush(&s_ctx.storage_manager, zic_log_timestamp());
                zic_publish_v2_outcome(&s_ctx, "irrigation.fault", "critical", "failed",
                                       "hydraulic_safety_shutdown", 0);
            }
            s_ctx.safety_fault_latched = true;
            s_ctx.scheduled_program_active = false;
            if (!irrigation_engine_is_idle(&s_ctx.engine)) {
                valve_diagnostics_command_close(&s_ctx.valve_diagnostics, now_ms);
            }
            (void)irrigation_engine_stop_all(&s_ctx.engine);
            (void)zic_controller_apply_event(&s_ctx.engine.controller, ZIC_EV_FAULT, -1);
            xEventGroupSetBits(s_runtime_events, ZIC_EVENT_FAULT);
        } else if (!s_ctx.safety_fault_latched) {
            xEventGroupClearBits(s_runtime_events, ZIC_EVENT_FAULT);
            zic_scheduler_advance_program(&s_ctx);
            zic_scheduler_start_due_program(&s_ctx);
        }

        uint32_t log_timestamp = zic_log_timestamp();
        if (log_timestamp != 0 &&
            (s_ctx.last_log_maintenance_timestamp == 0 ||
             log_timestamp - s_ctx.last_log_maintenance_timestamp >= 3600)) {
            s_ctx.last_log_maintenance_timestamp = log_timestamp;
            (void)storage_manager_prune_before(
                &s_ctx.storage_manager,
                log_timestamp > ZIC_LOG_RETENTION_SECONDS
                    ? log_timestamp - ZIC_LOG_RETENTION_SECONDS
                    : 0);
            if (storage_manager_flush_due(&s_ctx.storage_manager, log_timestamp, 3600)) {
                (void)storage_manager_flush(&s_ctx.storage_manager, log_timestamp);
            }
        }

        xSemaphoreGive(s_ctx_lock);
    }
}

static void zic_watersensor_task(void *arg)
{
    (void)arg;

    for (;;) {
        uint32_t delay_ms = WATERSENSOR_DEFAULT_OFFLINE_POLL_MS;
        if (xSemaphoreTake(s_watersensor_lock, pdMS_TO_TICKS(100)) == pdTRUE) {
            (void)watersensor_client_poll(&s_watersensor);
            delay_ms = watersensor_client_next_poll_ms(&s_watersensor);
            xSemaphoreGive(s_watersensor_lock);
        }
        vTaskDelay(pdMS_TO_TICKS(delay_ms));
    }
}

static void zic_telemetry_task(void *arg)
{
    (void)arg;
    char csv_line[128] = {0};

    xEventGroupWaitBits(s_runtime_events, ZIC_EVENT_ENGINE_READY, pdFALSE, pdTRUE, portMAX_DELAY);

    for (;;) {
        EventBits_t bits = xEventGroupGetBits(s_runtime_events);
        watersensor_link_state_t watersensor_state = WATERSENSOR_LINK_OFFLINE;
        if (xSemaphoreTake(s_watersensor_lock, pdMS_TO_TICKS(50)) == pdTRUE) {
            watersensor_state = watersensor_client_get_state(&s_watersensor);
            xSemaphoreGive(s_watersensor_lock);
        }

        xSemaphoreTake(s_ctx_lock, portMAX_DELAY);
        if (storage_manager_count(&s_ctx.storage_manager) > 0) {
            storage_manager_export_csv(&s_log_buffer[0], csv_line, sizeof(csv_line));
        } else {
            csv_line[0] = '\0';
        }

        ESP_LOGI(TAG,
                 "Telemetry controller_state=%d active_zone=%d alarm_high_flow=%d et_daily=%.2f fault=%d mqtt=%d watersensor=%d flow_source=%d logs=%u first_log=%s publish_topic=%s",
                 (int)s_ctx.engine.controller.state,
                 (int)s_ctx.engine.controller.active_zone,
                 alarm_manager_is_active(&s_ctx.alarm_manager, ZIC_ALARM_HIGH_FLOW),
                 s_ctx.et_output.daily_et_mm,
                 (bits & ZIC_EVENT_FAULT) != 0,
                 mqtt_transport_is_connected(&s_ctx.mqtt_transport),
                 (int)watersensor_state,
                 (int)s_ctx.flow_manager.source,
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

#if ZIC_RELAY_SELF_TEST
static void zic_run_relay_self_test(void)
{
    const TickType_t pulse_ticks = pdMS_TO_TICKS(500);
    const TickType_t gap_ticks = pdMS_TO_TICKS(250);

    ESP_LOGW(TAG, "RELAY SELF-TEST: no loads should be connected");
    if (hal_relay_init() != HAL_OK || hal_relay_all_off() != HAL_OK) {
        ESP_LOGE(TAG, "RELAY SELF-TEST aborted: MCP23017 initialization failed");
        (void)hal_relay_all_off();
        return;
    }

    for (uint8_t relay = 0; relay < HAL_RELAY_COUNT; ++relay) {
        if (hal_relay_all_off() != HAL_OK || hal_relay_on(relay) != HAL_OK) {
            ESP_LOGE(TAG, "RELAY SELF-TEST failed at relay %u", (unsigned)(relay + 1));
            (void)hal_relay_all_off();
            return;
        }

        ESP_LOGI(TAG, "RELAY SELF-TEST: relay %u/16 ON", (unsigned)(relay + 1));
        vTaskDelay(pulse_ticks);

        if (hal_relay_off(relay) != HAL_OK) {
            ESP_LOGE(TAG, "RELAY SELF-TEST could not turn relay %u OFF", (unsigned)(relay + 1));
            (void)hal_relay_all_off();
            return;
        }
        ESP_LOGI(TAG, "RELAY SELF-TEST: relay %u/16 OFF", (unsigned)(relay + 1));
        vTaskDelay(gap_ticks);
    }

    (void)hal_relay_all_off();
    ESP_LOGI(TAG, "RELAY SELF-TEST complete: all relays OFF");
}
#endif

void app_main(void)
{
    hmi_board_status_t hmi_status = {0};

    ESP_LOGI(TAG, "Zmartify Irrigation Controller booting");
    ota_manager_init();
#if ZIC_OTA_FORCE_HEALTH_FAILURE
    ESP_LOGE(TAG, "OTA ROLLBACK TEST IMAGE: health confirmation is forced to fail");
#endif

    esp_err_t nvs_err = nvs_flash_init();
    if (nvs_err == ESP_ERR_NVS_NO_FREE_PAGES || nvs_err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_err);

    ESP_ERROR_CHECK(esp_netif_init());
    (void)zic_wifi_init();

    if (!hmi_board_init()) {
        ESP_LOGW(TAG, "HMI init reported degraded status");
    }
    hmi_board_get_status(&hmi_status);
    ESP_LOGI(TAG,
             "HMI status: panel=%d touch=%d backlight=%d",
             hmi_status.panel_ready,
             hmi_status.touch_present,
             hmi_status.backlight_enabled);

#if ZIC_RELAY_SELF_TEST
    zic_run_relay_self_test();
    return;
#endif

    s_command_queue = xQueueCreate(ZIC_CMD_QUEUE_LEN, sizeof(zic_runtime_command_t));
    s_runtime_events = xEventGroupCreate();
    s_ctx_lock = xSemaphoreCreateMutex();
    s_watersensor_lock = xSemaphoreCreateMutex();
    if (s_command_queue == NULL || s_runtime_events == NULL || s_ctx_lock == NULL ||
        s_watersensor_lock == NULL) {
        ESP_LOGE(TAG, "Failed to allocate runtime resources");
        return;
    }

    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    esp_ota_img_states_t running_image_state = ESP_OTA_IMG_UNDEFINED;
    bool ota_confirmation_pending = running_partition != NULL &&
        esp_ota_get_state_partition(running_partition, &running_image_state) == ESP_OK &&
        running_image_state == ESP_OTA_IMG_PENDING_VERIFY;
    if (ota_confirmation_pending) {
        (void)ota_manager_transition(OTA_STATE_PENDING_CONFIRMATION);
        ESP_LOGW(TAG, "OTA image unconfirmed; irrigation inhibited until health check passes");
    } else {
        xEventGroupSetBits(s_runtime_events, ZIC_EVENT_OTA_CONFIRMED);
    }

    if (!event_bus_init()) {
        ESP_LOGW(TAG, "Event bus unavailable; Water Sensor events disabled");
    }
    if (hal_i2c_init() != HAL_OK) {
        ESP_LOGE(TAG, "I2C initialization failed; irrigation disabled");
        return;
    }
    if (hal_storage_init() != HAL_OK || config_manager_init() != CFG_OK) {
        ESP_LOGE(TAG, "Configuration initialization failed; irrigation disabled");
        return;
    }
    config_network_t network_config;
    if (config_get_network(&network_config) != CFG_OK ||
        hal_time_init(network_config.timezone) != HAL_OK) {
        ESP_LOGE(TAG, "Time initialization failed; automatic irrigation disabled");
        return;
    }
    config_system_t system_config;
    if (config_get_system(&system_config) != CFG_OK || hal_relay_init() != HAL_OK ||
        relay_manager_init((uint8_t)system_config.max_simultaneous_zones) != RELAY_OK) {
        ESP_LOGE(TAG, "Relay initialization failed; irrigation disabled");
        return;
    }
    hal_result_t pressure_result = hal_pressure_init();
    s_ctx.pressure_available = pressure_result == HAL_OK;
    if (!s_ctx.pressure_available) {
        ESP_LOGW(TAG, "Pressure sensor unavailable; pressure supervision disabled");
    }
    if (!watersensor_client_init(&s_watersensor, NULL)) {
        ESP_LOGW(TAG, "Water Sensor client unavailable; flow fallback remains active");
    } else {
        xTaskCreate(zic_watersensor_task, "watersensor", ZIC_WATERSENSOR_TASK_STACK,
                    NULL, ZIC_WATERSENSOR_TASK_PRIO, NULL);
    }

    xSemaphoreTake(s_ctx_lock, portMAX_DELAY);
    zic_runtime_init_context(&s_ctx);
    xSemaphoreGive(s_ctx_lock);

    xTaskCreate(zic_control_task, "zic_ctrl", ZIC_CTRL_TASK_STACK, NULL, ZIC_CTRL_TASK_PRIO, NULL);
    xTaskCreate(zic_telemetry_task, "zic_telem", ZIC_TELEM_TASK_STACK, NULL, ZIC_TELEM_TASK_PRIO, NULL);

    const diagnostics_manager_config_t diagnostics_config = {
        .snapshot = zic_diagnostics_snapshot,
        .raise_critical_alarm = zic_diagnostics_raise_critical,
        .ota_confirmed = zic_diagnostics_ota_confirmed,
        .audit = zic_ota_audit,
        .context = &s_ctx,
    };
    if (!diagnostics_manager_init(&diagnostics_config)) {
        ESP_LOGE(TAG, "Diagnostics Manager initialization failed; OTA images remain unconfirmed");
    }
}
