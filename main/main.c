#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_app_desc.h"
#include "esp_http_server.h"
#include "esp_mac.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "esp_timer.h"
#include "esp_vfs_fat.h"
#include "nvs_flash.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
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
#include "hmi_7b_ioexp.h"
#include "http_auth.h"
#include "irrigation_engine.h"
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
#include "version.h"
#include "zic_v2.h"
#include "psa/crypto.h"
#include <time.h>

#if __has_include("wifi_credentials.local.h")
#include "wifi_credentials.local.h"
#else
static const uint8_t zic_wifi_ssid[] = {0};
static const uint8_t zic_wifi_password[] = {0};
#endif

#if __has_include("http_auth.local.h")
#include "http_auth.local.h"
#else
static const uint8_t zic_http_admin_token_sha256[HTTP_AUTH_SHA256_LEN] = {0};
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
#define ZIC_CTRL_TASK_STACK 12288
#define ZIC_TELEM_TASK_STACK 6144
#define ZIC_WATERSENSOR_TASK_STACK 4096
#define ZIC_OTA_TASK_STACK 16384
#define ZIC_CTRL_TASK_PRIO 8
#define ZIC_TELEM_TASK_PRIO 5
#define ZIC_WATERSENSOR_TASK_PRIO 9

#define ZIC_RELAY_TEST_PULSE_MS 700u
#define ZIC_RELAY_TEST_GAP_MS 300u

#define ZIC_EVENT_ENGINE_READY BIT0
#define ZIC_EVENT_FAULT BIT1
#define ZIC_EVENT_OTA_CONFIRMED BIT2

#define ZIC_DEFAULT_BROKER_URI "mqtt://192.168.10.2:1883"
#define ZIC_NETWORK_HOSTNAME "zmartify-irrigation"
#define ZIC_SD_CARD_MOUNT_POINT "/sdcard"
#ifndef ZIC_SD_CARD_PIN_D0
#define ZIC_SD_CARD_PIN_D0 GPIO_NUM_13
#endif
#ifndef ZIC_SD_CARD_PIN_CMD
#define ZIC_SD_CARD_PIN_CMD GPIO_NUM_11
#endif
#ifndef ZIC_SD_CARD_PIN_CLK
#define ZIC_SD_CARD_PIN_CLK GPIO_NUM_12
#endif
#ifndef ZIC_SD_CARD_IOEXP_CS_BIT
#define ZIC_SD_CARD_IOEXP_CS_BIT 4
#endif

#define ZIC_HTTP_AUTH_FAILURE_LIMIT 5u
#define ZIC_HTTP_AUTH_FAILURE_WINDOW_US (60LL * 1000LL * 1000LL)
#define ZIC_OTA_UPLOAD_IDLE_TIMEOUT_US (30LL * 1000LL * 1000LL)
#define ZIC_MQTT_STALE_RECOVERY_MS 30000u

/* Device identity for Zmartify v2 topics; must match the edge registry
 * device_id assigned during onboarding (lowercase, hyphenated). */
#define ZIC_V2_DEVICE_ID "zmartify-irrigation-01"

typedef enum {
    ZIC_CMD_START_ZONE = 0,
    ZIC_CMD_RUN_PROGRAM,
    ZIC_CMD_SKIP_PROGRAM_STEP,
    ZIC_CMD_STOP_ZONE,
    ZIC_CMD_STOP_ALL,
    ZIC_CMD_SET_RAIN_DELAY,
    ZIC_CMD_CLEAR_RAIN_DELAY,
    ZIC_CMD_TRIGGER_OTA,
    ZIC_CMD_RELAY_SELF_TEST,
    ZIC_CMD_REPLACE_PROGRAMS_CONFIG,
    ZIC_CMD_CLEAR_PROGRAMS_CONFIG,
    ZIC_CMD_UPDATE_NETWORK_CONFIG,
    ZIC_CMD_INITIALIZE_SD_CARD
} zic_runtime_command_type_t;

typedef struct {
    uint32_t config_revision;
    uint8_t program_count;
    uint8_t schedule_count;
    char timezone[CONFIG_TZ_LEN];
    config_program_t programs[CONFIG_MAX_PROGRAMS];
} zic_program_sync_snapshot_t;

typedef struct {
    zic_runtime_command_type_t type;
    uint8_t zone_id;
    uint8_t program_id;
    uint32_t runtime_seconds;
    uint16_t rain_delay_hours;
    char command_id[ZIC_V2_COMMAND_ID_MAX];
    char ota_url[160];
    zic_program_sync_snapshot_t program_sync;
    config_network_t network_config;
    bool sd_format_if_needed;
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
    mqtt_transport_t mqtt_transport;
    bool mqtt_commands_authorized;
    bool scheduled_program_active;
    uint8_t scheduled_program_index;
    uint8_t scheduled_next_zone_index;
    config_program_t scheduled_program;
    char scheduled_program_command_id[ZIC_V2_COMMAND_ID_MAX];
    char active_zone_command_id[ZIC_V2_COMMAND_ID_MAX];
    uint32_t last_program_trigger_minute[CONFIG_MAX_PROGRAMS];
    flow_supervision_config_t flow_supervision;
    pressure_supervision_config_t pressure_supervision;
    config_hydraulic_t hydraulic_config;
    bool relay_available;
    bool pressure_available;
    bool storage_ready;
    bool storage_last_write_ok;
    bool safety_fault_latched;
    uint32_t scheduler_config_revision;
    uint32_t scheduler_program_count;
    uint32_t scheduler_schedule_count;
    uint32_t last_log_maintenance_timestamp;
    uint32_t last_weather_calculation_timestamp;
} zic_app_context_t;

static zic_app_context_t s_ctx;
static QueueHandle_t s_command_queue;
static EventGroupHandle_t s_runtime_events;
static SemaphoreHandle_t s_ctx_lock;
static SemaphoreHandle_t s_watersensor_lock;
static watersensor_client_t s_watersensor;
static TaskHandle_t s_control_task_handle;
static TaskHandle_t s_telemetry_task_handle;
static TaskHandle_t s_watersensor_task_handle;

static const char *s_command_topics[] = {
    "zmartify/v2/devices/" ZIC_V2_DEVICE_ID "/commands/irrigation/#",
};

static char s_v2_state_topic[128];
static char s_v2_outcome_topic[128];
static char s_diagnostics_topic[128];
static char s_v2_status_topic[128];
static char s_v2_state_payload[1024];
static char s_diagnostics_payload[768];
static zic_v2_command_tracker_t s_v2_command_tracker;
static alarm_manager_snapshot_t s_alarm_snapshot;
static httpd_handle_t s_ota_http_server;
static sdmmc_card_t *s_sd_card;
static char s_sd_card_last_error[96] = "not initialized";
static uint32_t s_http_auth_failures;
static int64_t s_http_auth_window_start_us;
static volatile uint32_t s_wifi_last_disconnect_reason;

static void zic_publish_v2_outcome(zic_app_context_t *ctx,
                                   const char *correlation_id,
                                   const char *event_type,
                                   const char *severity,
                                   const char *result,
                                   const char *detail,
                                   int zone_id);

static bool zic_enqueue_command(zic_runtime_command_t *cmd)
{
    if (cmd == NULL) {
        return false;
    }
    if (s_command_queue == NULL || xQueueSend(s_command_queue, &cmd, 0) != pdTRUE) {
        free(cmd);
        return false;
    }
    return true;
}

static void zic_mqtt_on_connected(void *user_ctx)
{
    zic_app_context_t *ctx = user_ctx;
    if (ctx != NULL) {
        (void)mqtt_transport_publish(&ctx->mqtt_transport, s_v2_status_topic,
                                     "{\"status\":\"online\"}",
                                     MQTT_TRANSPORT_QOS_STATE, true);
    }
}

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
                                     diag_policy_input_t *policy_input,
                                     uint8_t *active_alarms,
                                     uint32_t *log_entries)
{
    zic_app_context_t *ctx = context;
    if (ctx == NULL || policy_input == NULL || active_alarms == NULL ||
        log_entries == NULL || s_ctx_lock == NULL ||
        xSemaphoreTake(s_ctx_lock, pdMS_TO_TICKS(1000)) != pdTRUE) {
        return false;
    }

    *active_alarms = 0;
    policy_input->critical_alarms = 0;
    for (size_t index = 0; index < ZIC_MAX_ACTIVE_ALARMS; ++index) {
        const zic_alarm_t *alarm = &ctx->alarm_manager.alarms[index];
        if (alarm->active) {
            ++*active_alarms;
            if (alarm->severity == ZIC_ALARM_CRITICAL) {
                ++policy_input->critical_alarms;
            }
        }
    }
    *log_entries = (uint32_t)storage_manager_count(&ctx->storage_manager);
    EventBits_t runtime_bits = xEventGroupGetBits(s_runtime_events);
    policy_input->core_ready = (runtime_bits & ZIC_EVENT_ENGINE_READY) != 0 &&
        (runtime_bits & ZIC_EVENT_FAULT) == 0;
    policy_input->pressure_sensor_available = ctx->pressure_available;
    mqtt_transport_status_t mqtt_status = {0};
    mqtt_transport_get_status(&ctx->mqtt_transport, &mqtt_status);
    policy_input->mqtt_connected = mqtt_status.connected;
    policy_input->mqtt_connect_count = mqtt_status.connect_count;
    policy_input->mqtt_disconnect_count = mqtt_status.disconnect_count;
    policy_input->mqtt_error_count = mqtt_status.error_count;
    policy_input->mqtt_recovery_count = mqtt_status.recovery_count;
    (void)snprintf(policy_input->mqtt_last_error,
                   sizeof(policy_input->mqtt_last_error),
                   "%s",
                   mqtt_status.last_error);
    wifi_ap_record_t ap_info = {0};
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        policy_input->wifi_connected = true;
        policy_input->wifi_rssi_dbm = ap_info.rssi;
    }
    policy_input->wifi_last_disconnect_reason = s_wifi_last_disconnect_reason;
    policy_input->time_synchronized = hal_time_is_synced();
    policy_input->storage_ready = ctx->storage_ready;
    policy_input->storage_last_write_ok = ctx->storage_last_write_ok;
    policy_input->control_stack_free_bytes = s_control_task_handle != NULL
        ? uxTaskGetStackHighWaterMark(s_control_task_handle) * sizeof(StackType_t) : 0u;
    policy_input->telemetry_stack_free_bytes = s_telemetry_task_handle != NULL
        ? uxTaskGetStackHighWaterMark(s_telemetry_task_handle) * sizeof(StackType_t) : 0u;
    policy_input->watersensor_stack_free_bytes = s_watersensor_task_handle != NULL
        ? uxTaskGetStackHighWaterMark(s_watersensor_task_handle) * sizeof(StackType_t) : 0u;
#if ZIC_OTA_FORCE_HEALTH_FAILURE
    policy_input->core_ready = false;
#endif
    xSemaphoreGive(s_ctx_lock);

    policy_input->flow_sensor_available = false;
    if (s_watersensor_lock != NULL &&
        xSemaphoreTake(s_watersensor_lock, pdMS_TO_TICKS(100)) == pdTRUE) {
        policy_input->flow_sensor_available =
            watersensor_client_get_state(&s_watersensor) == WATERSENSOR_LINK_ONLINE;
        xSemaphoreGive(s_watersensor_lock);
    }
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
    zic_app_context_t *ctx = context;
    bool ok = hal_storage_write_blob("event_log", data, length) == HAL_OK;
    if (ctx != NULL) {
        ctx->storage_last_write_ok = ok;
    }
    return ok;
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

static void zic_alarm_restore(alarm_manager_t *manager)
{
    size_t length = sizeof(s_alarm_snapshot);
    hal_result_t result = hal_storage_read_blob("alarm_state", &s_alarm_snapshot, &length);
    if (result == HAL_OK &&
        (length != sizeof(s_alarm_snapshot) ||
         !alarm_manager_restore_snapshot(manager, &s_alarm_snapshot))) {
        ESP_LOGE(TAG, "Stored alarm state is corrupt; preserving safety lockout");
        alarm_manager_raise(manager, ZIC_ALARM_IRRIGATION_FAULT, ZIC_ALARM_CRITICAL);
    }
}

static void zic_alarm_flush(alarm_manager_t *manager)
{
    if (!alarm_manager_is_dirty(manager) ||
        !alarm_manager_export_snapshot(manager, &s_alarm_snapshot)) {
        return;
    }
    if (hal_storage_write_blob("alarm_state", &s_alarm_snapshot,
                               sizeof(s_alarm_snapshot)) == HAL_OK) {
        alarm_manager_mark_persisted(manager);
    } else {
        ESP_LOGE(TAG, "Failed to persist alarm safety state");
    }
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

static bool zic_ota_image_is_confirmed(void)
{
    const esp_partition_t *running_partition = esp_ota_get_running_partition();
    esp_ota_img_states_t image_state = ESP_OTA_IMG_UNDEFINED;
    if (running_partition == NULL ||
        esp_ota_get_state_partition(running_partition, &image_state) != ESP_OK) {
        return true;
    }
    return image_state != ESP_OTA_IMG_PENDING_VERIFY;
}

static bool zic_http_auth_sha256(const uint8_t *input,
                                 size_t input_len,
                                 uint8_t output[HTTP_AUTH_SHA256_LEN])
{
    size_t output_len = 0;
    return psa_hash_compute(PSA_ALG_SHA_256,
                            input,
                            input_len,
                            output,
                            HTTP_AUTH_SHA256_LEN,
                            &output_len) == PSA_SUCCESS &&
        output_len == HTTP_AUTH_SHA256_LEN;
}

static bool zic_http_auth_failure_is_limited(void)
{
    int64_t now_us = esp_timer_get_time();

    if (s_http_auth_window_start_us == 0 ||
        now_us - s_http_auth_window_start_us > ZIC_HTTP_AUTH_FAILURE_WINDOW_US) {
        s_http_auth_window_start_us = now_us;
        s_http_auth_failures = 0;
    }

    ++s_http_auth_failures;
    return s_http_auth_failures > ZIC_HTTP_AUTH_FAILURE_LIMIT;
}

static void zic_http_auth_reset_failures(void)
{
    s_http_auth_failures = 0;
    s_http_auth_window_start_us = 0;
}

static esp_err_t zic_http_require_admin(httpd_req_t *request)
{
    char authorization[128];
    size_t header_len = httpd_req_get_hdr_value_len(request, "Authorization");
    http_auth_result_t auth_result;

    if (header_len == 0u || header_len >= sizeof(authorization) ||
        httpd_req_get_hdr_value_str(request, "Authorization", authorization,
                                    sizeof(authorization)) != ESP_OK) {
        authorization[0] = '\0';
    }

    auth_result = http_auth_check_bearer(authorization,
                                         zic_http_admin_token_sha256,
                                         zic_http_auth_sha256);
    if (auth_result == HTTP_AUTH_RESULT_AUTHORIZED) {
        zic_http_auth_reset_failures();
        return ESP_OK;
    }

    ESP_LOGW(TAG, "HTTP admin authorization failed: %s", http_auth_result_name(auth_result));
    if (auth_result == HTTP_AUTH_RESULT_NOT_CONFIGURED) {
        httpd_resp_set_status(request, "503 Service Unavailable");
        httpd_resp_set_type(request, "text/plain");
        httpd_resp_sendstr(request, "HTTP admin auth not provisioned\n");
        return ESP_FAIL;
    }

    if (zic_http_auth_failure_is_limited()) {
        httpd_resp_set_status(request, "429 Too Many Requests");
        httpd_resp_set_type(request, "text/plain");
        httpd_resp_sendstr(request, "Too many authentication failures\n");
        return ESP_FAIL;
    }

    httpd_resp_set_hdr(request, "WWW-Authenticate", "Bearer realm=\"zmartify-admin\"");
    httpd_resp_send_err(request, HTTPD_401_UNAUTHORIZED, "Unauthorized");
    return ESP_FAIL;
}

static esp_err_t zic_http_require_content_type(httpd_req_t *request,
                                               const char *expected)
{
    char content_type[64];
    size_t header_len = httpd_req_get_hdr_value_len(request, "Content-Type");
    size_t expected_len = expected != NULL ? strlen(expected) : 0u;

    if (request == NULL || expected == NULL || expected_len == 0u ||
        header_len == 0u || header_len >= sizeof(content_type) ||
        httpd_req_get_hdr_value_str(request, "Content-Type", content_type,
                                    sizeof(content_type)) != ESP_OK ||
        strncmp(content_type, expected, expected_len) != 0 ||
        (content_type[expected_len] != '\0' && content_type[expected_len] != ';')) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Unsupported content type");
        return ESP_FAIL;
    }

    return ESP_OK;
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
    if (zic_http_require_admin(request) != ESP_OK) {
        return ESP_FAIL;
    }
    if (zic_http_require_content_type(request, "application/octet-stream") != ESP_OK) {
        return ESP_FAIL;
    }

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
    int64_t last_progress_us = esp_timer_get_time();
    while (remaining > 0) {
        int received = httpd_req_recv(request, buffer,
                                      remaining < (int)sizeof(buffer) ? remaining : (int)sizeof(buffer));
        if (received == HTTPD_SOCK_ERR_TIMEOUT) {
            if (esp_timer_get_time() - last_progress_us > ZIC_OTA_UPLOAD_IDLE_TIMEOUT_US) {
                ESP_LOGE(TAG, "Direct OTA upload stalled waiting for data");
                esp_ota_abort(ota_handle);
                (void)ota_manager_transition(OTA_STATE_FAILED);
                zic_ota_audit(&s_ctx, "ota direct upload timed out");
                httpd_resp_send_err(request, HTTPD_408_REQ_TIMEOUT, "OTA upload timed out");
                return ESP_FAIL;
            }
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
        last_progress_us = esp_timer_get_time();
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

static esp_err_t zic_reboot_http_handler(httpd_req_t *request)
{
    if (zic_http_require_admin(request) != ESP_OK) {
        return ESP_FAIL;
    }

    if (!zic_ota_image_is_confirmed()) {
        httpd_resp_set_status(request, "409 Conflict");
        httpd_resp_set_type(request, "text/plain");
        httpd_resp_sendstr(request, "OTA health confirmation pending\n");
        return ESP_FAIL;
    }
    if (!zic_ota_runtime_is_idle()) {
        httpd_resp_send_err(request, HTTPD_400_BAD_REQUEST, "Irrigation must be idle for reboot");
        return ESP_FAIL;
    }

    ESP_LOGI(TAG, "Software reboot requested via HTTP");
    httpd_resp_set_type(request, "text/plain");
    httpd_resp_sendstr(request, "Rebooting\n");
    xTaskCreate(zic_ota_restart_task, "http_reboot", 2048, NULL, 5, NULL);
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

static esp_err_t zic_health_http_handler(httpd_req_t *request)
{
    char payload[768];
    size_t length = diagnostics_health_to_json(payload, sizeof(payload));
    if (length == 0u) {
        const esp_app_desc_t *app_desc = esp_app_get_description();
        int written = snprintf(
            payload,
            sizeof(payload),
            "{\"firmware_version\":\"%s\",\"runtime\":\"healthy\","
            "\"communications\":\"%s\",\"mqtt_connected\":%s,\"time_synchronized\":%s,"
            "\"hydraulics\":\"unavailable\"}",
            app_desc != NULL ? app_desc->version : "unknown",
            mqtt_transport_is_connected(&s_ctx.mqtt_transport) ? "healthy" : "unavailable",
            mqtt_transport_is_connected(&s_ctx.mqtt_transport) ? "true" : "false",
            hal_time_is_synced() ? "true" : "false");
        if (written < 0 || (size_t)written >= sizeof(payload)) {
            return ESP_FAIL;
        }
        length = (size_t)written;
    }
    httpd_resp_set_type(request, "application/json");
    return httpd_resp_send(request, payload, length);
}

static esp_err_t zic_sd_card_mount(bool format_if_mount_failed)
{
    if (s_sd_card != NULL) {
        s_sd_card_last_error[0] = '\0';
        return ESP_OK;
    }

    if (!hmi_7b_ioexp_set_output_bit(ZIC_SD_CARD_IOEXP_CS_BIT, true)) {
        snprintf(s_sd_card_last_error, sizeof(s_sd_card_last_error), "sd cs expander control failed");
        return ESP_FAIL;
    }

    sdmmc_host_t host = SDMMC_HOST_DEFAULT();
    host.max_freq_khz = SDMMC_FREQ_DEFAULT;
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();
    slot_config.width = 1;
    slot_config.clk = ZIC_SD_CARD_PIN_CLK;
    slot_config.cmd = ZIC_SD_CARD_PIN_CMD;
    slot_config.d0 = ZIC_SD_CARD_PIN_D0;
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
        .format_if_mount_failed = format_if_mount_failed,
        .max_files = 4,
        .allocation_unit_size = 16 * 1024,
    };
    esp_err_t err = esp_vfs_fat_sdmmc_mount(ZIC_SD_CARD_MOUNT_POINT, &host, &slot_config, &mount_config, &s_sd_card);
    if (err != ESP_OK) {
        snprintf(s_sd_card_last_error, sizeof(s_sd_card_last_error), "mount failed: %s", esp_err_to_name(err));
        return err;
    }

    s_sd_card_last_error[0] = '\0';
    return ESP_OK;
}

static esp_err_t zic_sd_card_initialize(bool format)
{
    esp_err_t err = zic_sd_card_mount(false);
    if (err != ESP_OK && format) {
        err = zic_sd_card_mount(true);
    }
    if (err != ESP_OK || !format) {
        return err;
    }

    esp_vfs_fat_mount_config_t format_config = {
        .format_if_mount_failed = true,
        .max_files = 4,
        .allocation_unit_size = 16 * 1024,
    };
    err = esp_vfs_fat_sdcard_format_cfg(ZIC_SD_CARD_MOUNT_POINT, s_sd_card, &format_config);
    if (err != ESP_OK) {
        snprintf(s_sd_card_last_error, sizeof(s_sd_card_last_error), "format failed: %s", esp_err_to_name(err));
        return err;
    }

    FATFS *fs = NULL;
    DWORD free_clusters = 0;
    if (f_getfree(ZIC_SD_CARD_MOUNT_POINT, &free_clusters, &fs) != FR_OK || fs == NULL) {
        s_sd_card = NULL;
        err = zic_sd_card_mount(false);
        if (err != ESP_OK) {
            return err;
        }
    }

    s_sd_card_last_error[0] = '\0';
    return ESP_OK;
}

static void zic_sd_card_storage_snapshot(zic_v2_storage_t *storage)
{
    if (storage == NULL) {
        return;
    }
    memset(storage, 0, sizeof(*storage));
    storage->sd_card_mount_point = ZIC_SD_CARD_MOUNT_POINT;
    storage->sd_card_last_error = s_sd_card_last_error;

    storage->sd_card_mounted = s_sd_card != NULL;
    if (!storage->sd_card_mounted) {
        return;
    }
    storage->sd_card_total_bytes = (uint64_t)s_sd_card->csd.capacity * s_sd_card->csd.sector_size;

    FATFS *fs = NULL;
    DWORD free_clusters = 0;
    if (f_getfree(ZIC_SD_CARD_MOUNT_POINT, &free_clusters, &fs) == FR_OK && fs != NULL) {
        storage->sd_card_filesystem_total_bytes = (uint64_t)(fs->n_fatent - 2) * fs->csize * 512u;
        storage->sd_card_free_bytes = (uint64_t)free_clusters * fs->csize * 512u;
    }
    storage->sd_card_name = s_sd_card->cid.name;
}

static esp_err_t zic_sd_card_status_http_handler(httpd_req_t *request)
{
    (void)zic_sd_card_mount(false);

    uint64_t total_bytes = 0;
    uint64_t filesystem_total_bytes = 0;
    uint64_t free_bytes = 0;
    const char *state = s_sd_card != NULL ? "mounted" : "unavailable";
    if (s_sd_card != NULL) {
        total_bytes = (uint64_t)s_sd_card->csd.capacity * s_sd_card->csd.sector_size;
        FATFS *fs = NULL;
        DWORD free_clusters = 0;
        if (f_getfree(ZIC_SD_CARD_MOUNT_POINT, &free_clusters, &fs) == FR_OK && fs != NULL) {
            filesystem_total_bytes = (uint64_t)(fs->n_fatent - 2) * fs->csize * 512u;
            free_bytes = (uint64_t)free_clusters * fs->csize * 512u;
        }
    }

    cJSON *json = cJSON_CreateObject();
    if (json == NULL ||
        cJSON_AddStringToObject(json, "state", state) == NULL ||
        cJSON_AddBoolToObject(json, "mounted", s_sd_card != NULL) == NULL ||
        cJSON_AddNumberToObject(json, "total_bytes", (double)total_bytes) == NULL ||
        cJSON_AddNumberToObject(json, "card_total_bytes", (double)total_bytes) == NULL ||
        cJSON_AddNumberToObject(json, "filesystem_total_bytes", (double)filesystem_total_bytes) == NULL ||
        cJSON_AddNumberToObject(json, "free_bytes", (double)free_bytes) == NULL ||
        cJSON_AddStringToObject(json, "mount_point", ZIC_SD_CARD_MOUNT_POINT) == NULL ||
        cJSON_AddStringToObject(json, "last_error", s_sd_card_last_error) == NULL) {
        cJSON_Delete(json);
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "SD status response failed");
        return ESP_FAIL;
    }
    if (s_sd_card != NULL) {
        (void)cJSON_AddStringToObject(json, "card_name", s_sd_card->cid.name);
    }

    char *payload = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    if (payload == NULL) {
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "SD status response failed");
        return ESP_FAIL;
    }

    httpd_resp_set_type(request, "application/json");
    esp_err_t result = httpd_resp_sendstr(request, payload);
    cJSON_free(payload);
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

static esp_err_t zic_network_config_get_http_handler(httpd_req_t *request)
{
    config_network_t network;
    if (config_get_network(&network) != CFG_OK) {
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Config not ready");
        return ESP_FAIL;
    }

    cJSON *json = cJSON_CreateObject();
    if (json == NULL ||
        cJSON_AddStringToObject(json, "mqtt_broker_uri", network.mqtt_broker_uri) == NULL ||
        cJSON_AddNumberToObject(json, "mqtt_port", network.mqtt_port) == NULL ||
        cJSON_AddStringToObject(json, "mqtt_username", network.mqtt_username) == NULL ||
        cJSON_AddBoolToObject(json, "mqtt_password_configured", network.mqtt_password[0] != '\0') == NULL ||
        cJSON_AddBoolToObject(json, "mqtt_tls_enabled", network.mqtt_tls_enabled) == NULL ||
        cJSON_AddStringToObject(json, "ntp_server", network.ntp_server) == NULL ||
        cJSON_AddStringToObject(json, "timezone", network.timezone) == NULL) {
        cJSON_Delete(json);
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Config response failed");
        return ESP_FAIL;
    }

    char *payload = cJSON_PrintUnformatted(json);
    cJSON_Delete(json);
    if (payload == NULL) {
        httpd_resp_send_err(request, HTTPD_500_INTERNAL_SERVER_ERROR, "Config response failed");
        return ESP_FAIL;
    }

    httpd_resp_set_type(request, "application/json");
    esp_err_t result = httpd_resp_sendstr(request, payload);
    cJSON_free(payload);
    return result;
}

static esp_err_t zic_network_config_http_handler(httpd_req_t *request)
{
    if (zic_http_require_admin(request) != ESP_OK) {
        return ESP_FAIL;
    }
    if (zic_http_require_content_type(request, "application/json") != ESP_OK) {
        return ESP_FAIL;
    }

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
    if (config_set_network(&network) != CFG_OK ||
        config_manager_commit_network_recovery() != CFG_OK) {
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
    if (zic_http_require_admin(request) != ESP_OK) {
        return ESP_FAIL;
    }
    if (zic_http_require_content_type(request, "application/json") != ESP_OK) {
        return ESP_FAIL;
    }

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

    const httpd_uri_t reboot_uri = {
        .uri = "/reboot",
        .method = HTTP_POST,
        .handler = zic_reboot_http_handler,
    };
    err = httpd_register_uri_handler(s_ota_http_server, &reboot_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Reboot endpoint registration failed: %s", esp_err_to_name(err));
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

    const httpd_uri_t health_uri = {
        .uri = "/health",
        .method = HTTP_GET,
        .handler = zic_health_http_handler,
    };
    err = httpd_register_uri_handler(s_ota_http_server, &health_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Health endpoint registration failed: %s", esp_err_to_name(err));
        httpd_stop(s_ota_http_server);
        s_ota_http_server = NULL;
        return false;
    }

    const httpd_uri_t sd_card_status_uri = {
        .uri = "/storage/sd-card",
        .method = HTTP_GET,
        .handler = zic_sd_card_status_http_handler,
    };
    err = httpd_register_uri_handler(s_ota_http_server, &sd_card_status_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "SD-card status endpoint registration failed: %s", esp_err_to_name(err));
        httpd_stop(s_ota_http_server);
        s_ota_http_server = NULL;
        return false;
    }

    const httpd_uri_t network_config_get_uri = {
        .uri = "/config/network",
        .method = HTTP_GET,
        .handler = zic_network_config_get_http_handler,
    };
    err = httpd_register_uri_handler(s_ota_http_server, &network_config_get_uri);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Network config read endpoint registration failed: %s", esp_err_to_name(err));
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

    ESP_LOGI(TAG, "HTTP services ready: POST /ota, POST /reboot, GET /logs, GET /health, GET /storage/sd-card, GET/POST /config/network, POST /weather");
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
        s_wifi_last_disconnect_reason = event->reason;
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
    esp_netif_t *sta_netif;

    if (zic_wifi_ssid[0] == 0) {
        ESP_LOGW(TAG, "Wi-Fi credentials not provisioned; run scripts/configure-wifi.sh");
        return false;
    }

    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "esp_event_loop_create_default failed: %s", esp_err_to_name(err));
        return false;
    }

    sta_netif = esp_netif_create_default_wifi_sta();
    if (sta_netif == NULL) {
        ESP_LOGE(TAG, "Failed to create Wi-Fi STA interface");
        return false;
    }
    err = esp_netif_set_hostname(sta_netif, ZIC_NETWORK_HOSTNAME);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to set Wi-Fi hostname: %s", esp_err_to_name(err));
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
    if (esp_wifi_set_ps(WIFI_PS_NONE) != ESP_OK) {
        ESP_LOGW(TAG, "Failed to disable Wi-Fi power save");
    }

    ESP_LOGI(TAG, "Wi-Fi STA started for SSID '%s' as '%s'",
             (const char *)zic_wifi_ssid, ZIC_NETWORK_HOSTNAME);
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

static bool zic_uri_uses_tls(const char *uri)
{
    return uri != NULL && strncmp(uri, "mqtts://", strlen("mqtts://")) == 0;
}

static bool zic_normalize_broker_uri(config_network_t *network,
                                     char *normalized_uri,
                                     size_t normalized_uri_len)
{
    const char *raw_uri;
    const char *host_port;
    const char *scheme;
    int written;

    if (network == NULL || normalized_uri == NULL || normalized_uri_len == 0u) {
        return false;
    }

    raw_uri = network->mqtt_broker_uri;
    if (raw_uri[0] == '\0' || strstr(raw_uri, "://") != NULL) {
        return false;
    }

    host_port = raw_uri;
    while (*host_port == ':' || *host_port == '/') {
        ++host_port;
    }
    if (*host_port == '\0') {
        return false;
    }

    scheme = (network->mqtt_tls_enabled || network->mqtt_port == 8883u) ? "mqtts" : "mqtt";
    written = snprintf(normalized_uri, normalized_uri_len, "%s://%s", scheme, host_port);
    if (written <= 0 || (size_t)written >= normalized_uri_len) {
        return false;
    }
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
    if (!cJSON_IsNumber(item) || item->valuedouble < 0 || item->valuedouble > UINT32_MAX) {
        return false;
    }

    *value_out = (uint32_t)item->valuedouble;
    return item->valuedouble == (double)*value_out;
}

static bool zic_json_has_only(const cJSON *object,
                              const char *const *allowed,
                              size_t allowed_count)
{
    if (!cJSON_IsObject(object) || (size_t)cJSON_GetArraySize(object) != allowed_count) {
        return false;
    }
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, object) {
        bool found = false;
        for (size_t index = 0; index < allowed_count; ++index) {
            if (item->string != NULL && strcmp(item->string, allowed[index]) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

static bool zic_json_has_allowed(const cJSON *object,
                                 const char *const *allowed,
                                 size_t allowed_count)
{
    if (!cJSON_IsObject(object)) {
        return false;
    }
    const cJSON *item = NULL;
    cJSON_ArrayForEach(item, object) {
        bool found = false;
        for (size_t index = 0; index < allowed_count; ++index) {
            if (item->string != NULL && strcmp(item->string, allowed[index]) == 0) {
                found = true;
                break;
            }
        }
        if (!found) {
            return false;
        }
    }
    return true;
}

static bool zic_json_get_required_bool(const cJSON *root, const char *key, bool *value_out)
{
    const cJSON *item;

    if (root == NULL || key == NULL || value_out == NULL) {
        return false;
    }

    item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (!cJSON_IsBool(item)) {
        return false;
    }

    *value_out = cJSON_IsTrue(item);
    return true;
}

static bool zic_json_get_required_string(const cJSON *root,
                                         const char *key,
                                         char *out,
                                         size_t out_len)
{
    const cJSON *item;
    size_t len;

    if (root == NULL || key == NULL || out == NULL || out_len == 0u) {
        return false;
    }

    item = cJSON_GetObjectItemCaseSensitive(root, key);
    if (!cJSON_IsString(item) || item->valuestring == NULL) {
        return false;
    }

    len = strlen(item->valuestring);
    if (len == 0u || len >= out_len) {
        return false;
    }

    memcpy(out, item->valuestring, len + 1u);
    return true;
}

static bool zic_json_get_required_u32_range(const cJSON *root,
                                            const char *key,
                                            uint32_t min_value,
                                            uint32_t max_value,
                                            uint32_t *value_out)
{
    uint32_t value;

    if (!zic_get_json_u32(root, key, &value) || value < min_value || value > max_value) {
        return false;
    }

    *value_out = value;
    return true;
}

static bool zic_parse_zone_ref(const char *zone_ref, uint8_t *zone_index_out)
{
    char *end = NULL;
    unsigned long zone_number;

    if (zone_ref == NULL || zone_index_out == NULL || strncmp(zone_ref, "zone:", 5u) != 0) {
        return false;
    }

    zone_number = strtoul(zone_ref + 5u, &end, 10);
    if (end == NULL || *end != '\0' || zone_number == 0u || zone_number > CONFIG_MAX_ZONES) {
        return false;
    }

    *zone_index_out = (uint8_t)(zone_number - 1u);
    return true;
}

static bool zic_parse_local_time_hhmm(const char *value, uint8_t *hour_out, uint8_t *minute_out)
{
    if (value == NULL || hour_out == NULL || minute_out == NULL || strlen(value) != 5u ||
        value[2] != ':' || value[0] < '0' || value[0] > '9' || value[1] < '0' || value[1] > '9' ||
        value[3] < '0' || value[3] > '9' || value[4] < '0' || value[4] > '9') {
        return false;
    }

    *hour_out = (uint8_t)((value[0] - '0') * 10 + (value[1] - '0'));
    *minute_out = (uint8_t)((value[3] - '0') * 10 + (value[4] - '0'));
    return *hour_out < 24u && *minute_out < 60u;
}

static bool zic_weekday_number_to_mask(uint32_t weekday_number, config_days_t *mask_out)
{
    if (mask_out == NULL) {
        return false;
    }

    switch (weekday_number) {
    case 1u:
        *mask_out = CONFIG_DAY_MON;
        return true;
    case 2u:
        *mask_out = CONFIG_DAY_TUE;
        return true;
    case 3u:
        *mask_out = CONFIG_DAY_WED;
        return true;
    case 4u:
        *mask_out = CONFIG_DAY_THU;
        return true;
    case 5u:
        *mask_out = CONFIG_DAY_FRI;
        return true;
    case 6u:
        *mask_out = CONFIG_DAY_SAT;
        return true;
    case 7u:
        *mask_out = CONFIG_DAY_SUN;
        return true;
    default:
        return false;
    }
}

static bool zic_schedule_weekdays_mask(const cJSON *weekdays, config_days_t *mask_out)
{
    config_days_t mask = 0u;
    int count;

    if (!cJSON_IsArray(weekdays) || mask_out == NULL) {
        return false;
    }

    count = cJSON_GetArraySize(weekdays);
    if (count <= 0) {
        return false;
    }

    for (int index = 0; index < count; ++index) {
        const cJSON *item = cJSON_GetArrayItem(weekdays, index);
        uint32_t weekday_number;
        config_days_t day_mask;
        if (!cJSON_IsNumber(item) || item->valuedouble < 1.0 || item->valuedouble > 7.0 ||
            item->valuedouble != (double)(uint32_t)item->valuedouble) {
            return false;
        }
        weekday_number = (uint32_t)item->valuedouble;
        if (!zic_weekday_number_to_mask(weekday_number, &day_mask) || (mask & day_mask) != 0u) {
            return false;
        }
        mask |= day_mask;
    }

    *mask_out = mask;
    return true;
}

static bool zic_schedule_days_from_json(const cJSON *schedule,
                                        config_days_t *run_days_out,
                                        const char **detail_out)
{
    const cJSON *recurrence_type;
    const cJSON *weekdays;
    config_days_t explicit_mask = 0u;
    config_days_t derived_mask = 0u;
    bool has_explicit_mask = false;

    if (schedule == NULL || run_days_out == NULL) {
        if (detail_out != NULL) {
            *detail_out = "invalid_schedule";
        }
        return false;
    }

    recurrence_type = cJSON_GetObjectItemCaseSensitive(schedule, "recurrence_type");
    weekdays = cJSON_GetObjectItemCaseSensitive(schedule, "weekdays");
    if (!cJSON_IsString(recurrence_type) || recurrence_type->valuestring == NULL) {
        if (detail_out != NULL) {
            *detail_out = "invalid_schedule";
        }
        return false;
    }

    if (weekdays != NULL && !cJSON_IsNull(weekdays)) {
        if (!zic_schedule_weekdays_mask(weekdays, &explicit_mask)) {
            if (detail_out != NULL) {
                *detail_out = "invalid_schedule_weekdays";
            }
            return false;
        }
        has_explicit_mask = true;
    }

    if (strcmp(recurrence_type->valuestring, "daily") == 0) {
        derived_mask = CONFIG_DAYS_ALL;
    } else if (strcmp(recurrence_type->valuestring, "weekdays") == 0) {
        derived_mask = CONFIG_DAY_MON | CONFIG_DAY_TUE | CONFIG_DAY_WED |
            CONFIG_DAY_THU | CONFIG_DAY_FRI;
    } else if (strcmp(recurrence_type->valuestring, "weekends") == 0) {
        derived_mask = CONFIG_DAY_SAT | CONFIG_DAY_SUN;
    } else if (strcmp(recurrence_type->valuestring, "custom") == 0 ||
               strcmp(recurrence_type->valuestring, "weekly") == 0) {
        if (!has_explicit_mask) {
            if (detail_out != NULL) {
                *detail_out = "invalid_schedule_weekdays";
            }
            return false;
        }
        derived_mask = explicit_mask;
    } else {
        if (detail_out != NULL) {
            *detail_out = "unsupported_recurrence";
        }
        return false;
    }

    if (has_explicit_mask && explicit_mask != derived_mask) {
        if (detail_out != NULL) {
            *detail_out = "unsupported_schedule_variation";
        }
        return false;
    }

    *run_days_out = derived_mask;
    return true;
}

static bool zic_schedule_has_supported_constraints(const cJSON *schedule)
{
    const cJSON *interval_days;
    const cJSON *anchor_date;
    const cJSON *dates;

    if (schedule == NULL) {
        return false;
    }

    interval_days = cJSON_GetObjectItemCaseSensitive(schedule, "interval_days");
    if (interval_days != NULL && !cJSON_IsNull(interval_days)) {
        return false;
    }

    anchor_date = cJSON_GetObjectItemCaseSensitive(schedule, "anchor_date");
    if (anchor_date != NULL && !cJSON_IsNull(anchor_date) &&
        (!cJSON_IsString(anchor_date) || anchor_date->valuestring == NULL ||
         anchor_date->valuestring[0] != '\0')) {
        return false;
    }

    dates = cJSON_GetObjectItemCaseSensitive(schedule, "dates");
    if (dates != NULL && (!cJSON_IsArray(dates) || cJSON_GetArraySize(dates) != 0)) {
        return false;
    }

    return true;
}

static bool zic_parse_v2_programs_replace(const cJSON *parameters,
                                          zic_program_sync_snapshot_t *snapshot,
                                          const char **detail_out)
{
    static const char *const parameter_fields[] = {
        "config_revision", "timezone", "programs"
    };
    static const char *const program_fields[] = {
        "program_id", "name", "enabled", "seasonal_adjust_pct",
        "weather_mode", "zones", "schedules"
    };
    static const char *const zone_fields[] = {
        "zone_ref", "sort_order", "duration_seconds", "enabled"
    };
    static const char *const schedule_fields[] = {
        "schedule_id", "name", "enabled", "recurrence_type",
        "weekdays", "start_local_time", "interval_days", "anchor_date", "dates"
    };
    const cJSON *programs;
    uint32_t config_revision;
    int program_count;
    char seen_program_ids[CONFIG_MAX_PROGRAMS][48] = {{0}};
    char seen_schedule_ids[CONFIG_MAX_PROGRAMS * CONFIG_MAX_START_TIMES][48] = {{0}};
    size_t seen_schedule_count = 0u;

    if (detail_out != NULL) {
        *detail_out = "invalid_schema";
    }
    if (!zic_json_has_only(parameters, parameter_fields,
                           sizeof(parameter_fields) / sizeof(parameter_fields[0])) ||
        snapshot == NULL ||
        !zic_json_get_required_u32_range(parameters, "config_revision", 0u, UINT32_MAX,
                                         &config_revision) ||
        !zic_json_get_required_string(parameters, "timezone", snapshot->timezone,
                                      sizeof(snapshot->timezone))) {
        return false;
    }

    programs = cJSON_GetObjectItemCaseSensitive(parameters, "programs");
    if (!cJSON_IsArray(programs)) {
        if (detail_out != NULL) {
            *detail_out = "invalid_programs";
        }
        return false;
    }

    program_count = cJSON_GetArraySize(programs);
    if (program_count < 0 || program_count > CONFIG_MAX_PROGRAMS) {
        if (detail_out != NULL) {
            *detail_out = "invalid_programs";
        }
        return false;
    }

    memset(snapshot->programs, 0, sizeof(snapshot->programs));
    snapshot->config_revision = config_revision;
    snapshot->program_count = (uint8_t)program_count;
    snapshot->schedule_count = 0u;

    for (int program_index = 0; program_index < program_count; ++program_index) {
        const cJSON *program_json = cJSON_GetArrayItem(programs, program_index);
        const cJSON *zones;
        const cJSON *schedules;
        char program_id[48];
        char weather_mode[16];
        uint32_t seasonal_adjust_pct;
        bool program_enabled;
        int schedule_count;
        config_program_t *program = &snapshot->programs[program_index];
        bool have_run_days = false;

        if (!zic_json_has_only(program_json, program_fields,
                               sizeof(program_fields) / sizeof(program_fields[0])) ||
            !zic_json_get_required_string(program_json, "program_id", program_id,
                                          sizeof(program_id)) ||
            !zic_json_get_required_string(program_json, "name", program->name,
                                          sizeof(program->name)) ||
            !zic_json_get_required_bool(program_json, "enabled", &program_enabled) ||
            !zic_json_get_required_u32_range(program_json, "seasonal_adjust_pct", 0u, 200u,
                                             &seasonal_adjust_pct) ||
            !zic_json_get_required_string(program_json, "weather_mode", weather_mode,
                                          sizeof(weather_mode))) {
            if (detail_out != NULL) {
                *detail_out = "invalid_program";
            }
            return false;
        }

        for (int seen_index = 0; seen_index < program_index; ++seen_index) {
            if (strcmp(seen_program_ids[seen_index], program_id) == 0) {
                if (detail_out != NULL) {
                    *detail_out = "duplicate_program_id";
                }
                return false;
            }
        }
        memcpy(seen_program_ids[program_index], program_id, strlen(program_id) + 1u);

        program->enabled = program_enabled;
        program->seasonal_adjust_pct = (uint8_t)seasonal_adjust_pct;
        if (strcmp(weather_mode, "automatic") == 0) {
            program->weather_skip_enabled = true;
            program->rain_skip_threshold_pct = 50u;
        } else if (strcmp(weather_mode, "disabled") == 0) {
            program->weather_skip_enabled = false;
            program->rain_skip_threshold_pct = 0u;
        } else {
            if (detail_out != NULL) {
                *detail_out = "unsupported_weather_mode";
            }
            return false;
        }

        zones = cJSON_GetObjectItemCaseSensitive(program_json, "zones");
        schedules = cJSON_GetObjectItemCaseSensitive(program_json, "schedules");
        if (!cJSON_IsArray(zones) || !cJSON_IsArray(schedules) ||
            cJSON_GetArraySize(zones) > CONFIG_MAX_ZONES) {
            if (detail_out != NULL) {
                *detail_out = "invalid_program";
            }
            return false;
        }

        bool zone_seen[CONFIG_MAX_ZONES] = {false};
        uint8_t group_counts[CONFIG_MAX_ZONES] = {0};
        for (int zone_item_index = 0; zone_item_index < cJSON_GetArraySize(zones); ++zone_item_index) {
            const cJSON *zone_json = cJSON_GetArrayItem(zones, zone_item_index);
            char zone_ref[16];
            uint32_t sort_order;
            uint32_t duration_seconds;
            bool zone_enabled;
            uint8_t zone_index;
            if (!zic_json_has_only(zone_json, zone_fields,
                                   sizeof(zone_fields) / sizeof(zone_fields[0])) ||
                !zic_json_get_required_string(zone_json, "zone_ref", zone_ref, sizeof(zone_ref)) ||
                !zic_json_get_required_u32_range(zone_json, "sort_order", 1u, CONFIG_MAX_ZONES,
                                                 &sort_order) ||
                !zic_json_get_required_u32_range(zone_json, "duration_seconds", 60u,
                                                 (uint32_t)UINT8_MAX * 60u,
                                                 &duration_seconds) ||
                !zic_json_get_required_bool(zone_json, "enabled", &zone_enabled) ||
                !zic_parse_zone_ref(zone_ref, &zone_index) ||
                (duration_seconds % 60u) != 0u) {
                if (detail_out != NULL) {
                    *detail_out = "invalid_zone_definition";
                }
                return false;
            }
            if (zone_seen[zone_index] || ++group_counts[sort_order - 1u] > 2u) {
                if (detail_out != NULL) {
                    *detail_out = "duplicate_zone_entry";
                }
                return false;
            }
            zone_seen[zone_index] = true;
            program->zone_runtime_min[zone_index] = zone_enabled
                ? (uint8_t)(duration_seconds / 60u)
                : 0u;
            program->zone_group[zone_index] = (uint8_t)sort_order;
        }


        schedule_count = cJSON_GetArraySize(schedules);
        if (schedule_count < 0 || schedule_count > CONFIG_MAX_START_TIMES) {
            if (detail_out != NULL) {
                *detail_out = "invalid_schedule";
            }
            return false;
        }

        for (int schedule_index = 0; schedule_index < schedule_count; ++schedule_index) {
            const cJSON *schedule_json = cJSON_GetArrayItem(schedules, schedule_index);
            char schedule_id[48];
            char start_local_time[8];
            config_days_t schedule_run_days;
            bool schedule_enabled;
            uint8_t hour;
            uint8_t minute;

            if (!zic_json_has_only(schedule_json, schedule_fields,
                                   sizeof(schedule_fields) / sizeof(schedule_fields[0])) ||
                !zic_json_get_required_string(schedule_json, "schedule_id", schedule_id,
                                              sizeof(schedule_id)) ||
                !zic_json_get_required_bool(schedule_json, "enabled", &schedule_enabled) ||
                !zic_json_get_required_string(schedule_json, "start_local_time", start_local_time,
                                              sizeof(start_local_time)) ||
                !zic_parse_local_time_hhmm(start_local_time, &hour, &minute) ||
                !zic_schedule_has_supported_constraints(schedule_json) ||
                !zic_schedule_days_from_json(schedule_json, &schedule_run_days, detail_out)) {
                if (detail_out != NULL && *detail_out == NULL) {
                    *detail_out = "invalid_schedule";
                }
                return false;
            }

            for (size_t seen_index = 0; seen_index < seen_schedule_count; ++seen_index) {
                if (strcmp(seen_schedule_ids[seen_index], schedule_id) == 0) {
                    if (detail_out != NULL) {
                        *detail_out = "duplicate_schedule_id";
                    }
                    return false;
                }
            }
            memcpy(seen_schedule_ids[seen_schedule_count], schedule_id,
                   strlen(schedule_id) + 1u);
            ++seen_schedule_count;

            program->start_times[schedule_index].enabled = schedule_enabled;
            program->start_times[schedule_index].hour = hour;
            program->start_times[schedule_index].minute = minute;
            if (!schedule_enabled) {
                continue;
            }
            if (!have_run_days) {
                program->run_days = schedule_run_days;
                have_run_days = true;
            } else if (program->run_days != schedule_run_days) {
                if (detail_out != NULL) {
                    *detail_out = "unsupported_schedule_variation";
                }
                return false;
            }
        }

        snapshot->schedule_count = (uint8_t)(snapshot->schedule_count + schedule_count);
    }

    return true;
}

static void zic_runtime_apply_timezone(const char *timezone)
{
    if (timezone == NULL || timezone[0] == '\0') {
        return;
    }

    setenv("TZ", timezone, 1);
    tzset();
    ESP_LOGI(TAG, "Runtime timezone updated: %s", timezone);
}

static void zic_runtime_reset_scheduler_state(zic_app_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    ctx->scheduled_program_active = false;
    ctx->scheduled_program_index = 0u;
    ctx->scheduled_next_zone_index = 0u;
    memset(&ctx->scheduled_program, 0, sizeof(ctx->scheduled_program));
    memset(ctx->scheduled_program_command_id, 0, sizeof(ctx->scheduled_program_command_id));
    memset(ctx->active_zone_command_id, 0, sizeof(ctx->active_zone_command_id));
    memset(ctx->last_program_trigger_minute, 0, sizeof(ctx->last_program_trigger_minute));
}

static void zic_scheduler_load_metadata(zic_app_context_t *ctx)
{
    if (ctx == NULL) {
        return;
    }

    (void)persistent_store_get_u32("sched_cfg_rev", &ctx->scheduler_config_revision, 0u);
    (void)persistent_store_get_u32("sched_prog_ct", &ctx->scheduler_program_count, 0u);
    (void)persistent_store_get_u32("sched_sched_ct", &ctx->scheduler_schedule_count, 0u);
}

static void zic_scheduler_store_metadata(zic_app_context_t *ctx,
                                         uint32_t config_revision,
                                         uint32_t program_count,
                                         uint32_t schedule_count)
{
    if (ctx == NULL) {
        return;
    }

    ctx->scheduler_config_revision = config_revision;
    ctx->scheduler_program_count = program_count;
    ctx->scheduler_schedule_count = schedule_count;
    (void)persistent_store_set_u32("sched_cfg_rev", config_revision);
    (void)persistent_store_set_u32("sched_prog_ct", program_count);
    (void)persistent_store_set_u32("sched_sched_ct", schedule_count);
}

static bool zic_scheduler_next_run_iso(char *out, size_t out_len,
                                       char *program_name, size_t program_name_len)
{
    time_t now;
    time_t best = 0;
    bool found = false;

    if (out == NULL || out_len == 0u || !hal_time_is_synced()) {
        return false;
    }

    now = time(NULL);
    for (uint8_t program_index = 0; program_index < CONFIG_MAX_PROGRAMS; ++program_index) {
        config_program_t program;
        if (config_get_program(program_index, &program) != CFG_OK || !program.enabled) {
            continue;
        }
        for (uint8_t start_index = 0; start_index < CONFIG_MAX_START_TIMES; ++start_index) {
            const config_start_time_t *start = &program.start_times[start_index];
            if (!start->enabled) {
                continue;
            }
            for (int day_offset = 0; day_offset <= 7; ++day_offset) {
                struct tm candidate_tm;
                localtime_r(&now, &candidate_tm);
                candidate_tm.tm_sec = 0;
                candidate_tm.tm_min = start->minute;
                candidate_tm.tm_hour = start->hour;
                candidate_tm.tm_mday += day_offset;
                candidate_tm.tm_isdst = -1;
                time_t candidate = mktime(&candidate_tm);
                if (candidate <= now) {
                    continue;
                }
                if ((program.run_days & (config_days_t)(1u << candidate_tm.tm_wday)) == 0u) {
                    continue;
                }
                if (!found || candidate < best) {
                    best = candidate;
                    found = true;
                    if (program_name != NULL && program_name_len > 0u) {
                        snprintf(program_name, program_name_len, "%s", program.name);
                    }
                }
            }
        }
    }

    if (!found) {
        return false;
    }

    struct tm tm_utc;
    gmtime_r(&best, &tm_utc);
    return strftime(out, out_len, "%Y-%m-%dT%H:%M:%SZ", &tm_utc) > 0u;
}

static const char *zic_scheduler_blocked_reason(zic_app_context_t *ctx)
{
    uint32_t rain_delay = 0u;
    config_system_t system;

    if (ctx == NULL) {
        return NULL;
    }
    if (alarm_manager_has_lockout(&ctx->alarm_manager) || ctx->safety_fault_latched) {
        return "fault";
    }
    if (config_get_system(&system) == CFG_OK && system.operational_mode != CONFIG_MODE_AUTO) {
        return "controller_mode";
    }
    (void)persistent_store_get_u32("rain_delay_h", &rain_delay, 0u);
    if (rain_delay > 0u) {
        return "rain_delay";
    }
    if (!hal_time_is_synced()) {
        return "time_unsynced";
    }
    if (!irrigation_engine_is_idle(&ctx->engine) && !ctx->scheduled_program_active) {
        return "manual_run";
    }
    return NULL;
}

static void zic_mqtt_on_message(const char *topic,
                                size_t topic_len,
                                const char *payload,
                                size_t payload_len,
                                void *user_ctx)
{
    zic_runtime_command_t *cmd;
    uint32_t value;
    uint32_t runtime_seconds;
    char topic_buf[128];
    char json_scratch_stack[256];
    char *json_scratch = json_scratch_stack;
    cJSON *json = NULL;
    const char *schema_error_detail = "invalid_schema";

    (void)user_ctx;
    if (s_command_queue == NULL || topic == NULL || topic_len >= sizeof(topic_buf) ||
        payload_len == 0u) {
        return;
    }

    if (payload_len >= sizeof(json_scratch_stack)) {
        json_scratch = malloc(payload_len + 1u);
        if (json_scratch == NULL) {
            ESP_LOGW(TAG, "Dropping MQTT command: payload scratch allocation failed (%u bytes)",
                     (unsigned)payload_len);
            return;
        }
    }

    cmd = calloc(1u, sizeof(*cmd));
    if (cmd == NULL) {
        if (json_scratch != json_scratch_stack) {
            free(json_scratch);
        }
        ESP_LOGW(TAG, "Dropping MQTT command: command allocation failed");
        return;
    }
    value = 0;
    runtime_seconds = 300;
    zic_copy_to_cstr(topic_buf, sizeof(topic_buf), topic, topic_len);

    /* Zmartify v2 is the sole active command contract. */
    const char *v2_prefix = "zmartify/v2/devices/" ZIC_V2_DEVICE_ID "/commands/irrigation/";
    if (strncmp(topic_buf, v2_prefix, strlen(v2_prefix)) == 0) {
        const char *action = topic_buf + strlen(v2_prefix);
        ESP_LOGI(TAG, "Received v2 command action=%s payload_len=%u",
             action, (unsigned)payload_len);
        json = zic_parse_json_payload(payload, payload_len, json_scratch, payload_len + 1u);
        if (!cJSON_IsObject(json)) {
            ESP_LOGW(TAG, "Invalid v2 command payload for %s", action);
            cJSON_Delete(json);
            if (json_scratch != json_scratch_stack) {
                free(json_scratch);
            }
            zic_publish_v2_outcome(&s_ctx, NULL, "command.rejected", "warning",
                                   "rejected", "invalid_json", 0);
            return;
        }

        const cJSON *command_id = cJSON_GetObjectItemCaseSensitive(json, "command_id");
        const cJSON *source_timestamp = cJSON_GetObjectItemCaseSensitive(json, "source_timestamp");
        const cJSON *parameters = cJSON_GetObjectItemCaseSensitive(json, "parameters");
        zic_v2_command_t v2_command = {
            .authorized = s_ctx.mqtt_commands_authorized,
        };
        if (cJSON_IsString(command_id) && command_id->valuestring != NULL &&
            strlen(command_id->valuestring) < sizeof(v2_command.command_id)) {
            strncpy(v2_command.command_id, command_id->valuestring,
                    sizeof(v2_command.command_id) - 1u);
        }
        if (cJSON_IsString(source_timestamp) && source_timestamp->valuestring != NULL) {
            (void)zic_v2_parse_utc_timestamp(source_timestamp->valuestring,
                                             &v2_command.source_epoch_s);
        }

        static const char *const envelope_fields[] = {
            "command_id", "source_timestamp", "parameters"
        };
        bool schema_valid = zic_json_has_only(json, envelope_fields,
                                              sizeof(envelope_fields) / sizeof(envelope_fields[0]));
        if (strcmp(action, "zone/start") == 0) {
            static const char *const fields[] = {"zone_id", "duration_seconds"};
            v2_command.action = ZIC_V2_ACTION_ZONE_START;
            schema_valid = schema_valid && zic_json_has_only(parameters, fields, 2u) &&
                zic_get_json_u32(parameters, "zone_id", &value) &&
                zic_get_json_u32(parameters, "duration_seconds", &runtime_seconds);
            v2_command.zone_id = value;
            v2_command.runtime_seconds = runtime_seconds;
            cmd->type = ZIC_CMD_START_ZONE;
            cmd->zone_id = (uint8_t)value;
            cmd->runtime_seconds = runtime_seconds;
        } else if (strcmp(action, "zone/stop") == 0) {
            static const char *const fields[] = {"zone_id"};
            v2_command.action = ZIC_V2_ACTION_ZONE_STOP;
            schema_valid = schema_valid && zic_json_has_only(parameters, fields, 1u) &&
                zic_get_json_u32(parameters, "zone_id", &value);
            v2_command.zone_id = value;
            cmd->type = ZIC_CMD_STOP_ZONE;
            cmd->zone_id = (uint8_t)value;
        } else if (strcmp(action, "program/start") == 0) {
            static const char *const fields[] = {"program_id"};
            v2_command.action = ZIC_V2_ACTION_PROGRAM_START;
            schema_valid = schema_valid && zic_json_has_only(parameters, fields, 1u) &&
                zic_get_json_u32(parameters, "program_id", &value);
            v2_command.program_id = value;
            cmd->type = ZIC_CMD_RUN_PROGRAM;
            cmd->program_id = (uint8_t)value;
        } else if (strcmp(action, "program/skip") == 0) {
            v2_command.action = ZIC_V2_ACTION_PROGRAM_SKIP;
            schema_valid = schema_valid && zic_json_has_only(parameters, NULL, 0u);
            cmd->type = ZIC_CMD_SKIP_PROGRAM_STEP;
        } else if (strcmp(action, "stop_all") == 0) {
            v2_command.action = ZIC_V2_ACTION_STOP_ALL;
            schema_valid = schema_valid && zic_json_has_only(parameters, NULL, 0u);
            cmd->type = ZIC_CMD_STOP_ALL;
        } else if (strcmp(action, "rain_delay") == 0) {
            static const char *const fields[] = {"delay_hours"};
            v2_command.action = ZIC_V2_ACTION_RAIN_DELAY;
            schema_valid = schema_valid && zic_json_has_only(parameters, fields, 1u) &&
                zic_get_json_u32(parameters, "delay_hours", &value);
            v2_command.rain_delay_hours = value;
            cmd->type = value == 0u ? ZIC_CMD_CLEAR_RAIN_DELAY : ZIC_CMD_SET_RAIN_DELAY;
            cmd->rain_delay_hours = (uint16_t)value;
        } else if (strcmp(action, "config/programs/replace") == 0) {
            v2_command.action = ZIC_V2_ACTION_CONFIG_PROGRAMS_REPLACE;
            schema_valid = schema_valid &&
                zic_parse_v2_programs_replace(parameters, &cmd->program_sync, &schema_error_detail);
            cmd->type = ZIC_CMD_REPLACE_PROGRAMS_CONFIG;
        } else if (strcmp(action, "config/programs/clear") == 0) {
            static const char *const fields[] = {"config_revision"};
            v2_command.action = ZIC_V2_ACTION_CONFIG_PROGRAMS_CLEAR;
            schema_valid = schema_valid && zic_json_has_only(parameters, fields, 1u) &&
                zic_json_get_required_u32_range(parameters, "config_revision", 0u, UINT32_MAX,
                                                &cmd->program_sync.config_revision);
            cmd->type = ZIC_CMD_CLEAR_PROGRAMS_CONFIG;
            cmd->program_sync.program_count = 0u;
            cmd->program_sync.schedule_count = 0u;
            cmd->program_sync.timezone[0] = '\0';
        } else if (strcmp(action, "config/network") == 0) {
            static const char *const fields[] = {
                "mqtt_broker_uri", "mqtt_uri", "mqtt_username", "mqtt_password",
                "mqtt_tls_enabled", "mqtt_port", "ntp_server", "timezone"
            };
            config_network_t network;
            v2_command.action = ZIC_V2_ACTION_CONFIG_NETWORK;
            schema_valid = schema_valid && zic_json_has_allowed(parameters, fields,
                sizeof(fields) / sizeof(fields[0])) &&
                config_get_network(&network) == CFG_OK;
            if (schema_valid) {
                schema_valid = schema_valid && zic_json_copy_string(parameters, "mqtt_broker_uri", network.mqtt_broker_uri, sizeof(network.mqtt_broker_uri));
                schema_valid = schema_valid && zic_json_copy_string(parameters, "mqtt_uri", network.mqtt_broker_uri, sizeof(network.mqtt_broker_uri));
                schema_valid = schema_valid && zic_json_copy_string(parameters, "mqtt_username", network.mqtt_username, sizeof(network.mqtt_username));
                schema_valid = schema_valid && zic_json_copy_string(parameters, "mqtt_password", network.mqtt_password, sizeof(network.mqtt_password));
                schema_valid = schema_valid && zic_json_copy_string(parameters, "ntp_server", network.ntp_server, sizeof(network.ntp_server));
                schema_valid = schema_valid && zic_json_copy_string(parameters, "timezone", network.timezone, sizeof(network.timezone));
                const cJSON *tls = cJSON_GetObjectItemCaseSensitive(parameters, "mqtt_tls_enabled");
                if (cJSON_IsBool(tls)) {
                    network.mqtt_tls_enabled = cJSON_IsTrue(tls);
                }
                const cJSON *port = cJSON_GetObjectItemCaseSensitive(parameters, "mqtt_port");
                if (port != NULL) {
                    schema_valid = schema_valid && cJSON_IsNumber(port) &&
                        port->valuedouble >= 0 && port->valuedouble <= 65535 &&
                        port->valuedouble == (double)(uint16_t)port->valuedouble;
                    if (schema_valid) {
                        network.mqtt_port = (uint16_t)port->valuedouble;
                    }
                }
            }
            cmd->type = ZIC_CMD_UPDATE_NETWORK_CONFIG;
            cmd->network_config = network;
        } else if (strcmp(action, "config/storage/sd-card/initialize") == 0) {
            static const char *const fields[] = {"format"};
            v2_command.action = ZIC_V2_ACTION_CONFIG_STORAGE_SD_CARD_INITIALIZE;
            schema_valid = schema_valid && zic_json_has_allowed(parameters, fields, 1u);
            const cJSON *format = cJSON_GetObjectItemCaseSensitive(parameters, "format");
            cmd->type = ZIC_CMD_INITIALIZE_SD_CARD;
            cmd->sd_format_if_needed = format == NULL || cJSON_IsTrue(format);
        } else {
            schema_valid = false;
        }

        uint32_t now_epoch_s = 0;
        (void)hal_time_get_epoch(&now_epoch_s);
        zic_v2_command_reason_t reason = ZIC_V2_REASON_INVALID_TIMESTAMP;
        zic_v2_command_decision_t decision = schema_valid
            ? zic_v2_validate_command(&v2_command, now_epoch_s,
                                      &s_v2_command_tracker, &reason)
            : ZIC_V2_COMMAND_REJECTED;
        if (!schema_valid) {
            reason = ZIC_V2_REASON_INVALID_TIMESTAMP;
        }
        (void)snprintf(cmd->command_id,
               sizeof(cmd->command_id),
                   "%s",
                   v2_command.command_id);
        cJSON_Delete(json);
        if (json_scratch != json_scratch_stack) {
            free(json_scratch);
            json_scratch = json_scratch_stack;
        }

        if (decision == ZIC_V2_COMMAND_DUPLICATE) {
            free(cmd);
            zic_publish_v2_outcome(&s_ctx, v2_command.command_id, "command.duplicate",
                                   "warning", "duplicate", "already_processed", 0);
            return;
        }
        if (decision == ZIC_V2_COMMAND_REJECTED) {
            free(cmd);
            zic_publish_v2_outcome(&s_ctx, v2_command.command_id, "command.rejected",
                                   "warning", "rejected",
                                   schema_valid ? zic_v2_command_reason_name(reason) : schema_error_detail, 0);
            return;
        }
        if (!zic_enqueue_command(cmd)) {
            zic_publish_v2_outcome(&s_ctx, v2_command.command_id, "command.rejected",
                                   "warning", "rejected", "queue_full", 0);
            return;
        }
        zic_publish_v2_outcome(&s_ctx, v2_command.command_id, "command.accepted",
                               "info", "accepted", NULL, 0);
        return;
    }

    if (json_scratch != json_scratch_stack) {
        free(json_scratch);
    }
}

static void zic_runtime_init_context(zic_app_context_t *ctx)
{
    persistent_store_init();
    zic_scheduler_load_metadata(ctx);

    irrigation_engine_init(&ctx->engine);
    alarm_manager_init(&ctx->alarm_manager);
    zic_alarm_restore(&ctx->alarm_manager);
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
    ctx->storage_ready = storage_manager_init(&ctx->storage_manager, s_log_buffer,
                                              ZIC_LOG_PERSIST_CAPACITY);
    ctx->storage_last_write_ok = ctx->storage_ready;
    storage_manager_set_persistence(&ctx->storage_manager, zic_log_load, zic_log_save, ctx);
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
    zic_v2_state_topic(ZIC_V2_DEVICE_ID, s_v2_state_topic, sizeof(s_v2_state_topic));
    zic_v2_outcome_topic(ZIC_V2_DEVICE_ID, s_v2_outcome_topic, sizeof(s_v2_outcome_topic));
    snprintf(s_diagnostics_topic, sizeof(s_diagnostics_topic),
             "zmartify/v2/devices/%s/diagnostics/health", ZIC_V2_DEVICE_ID);
    snprintf(s_v2_status_topic, sizeof(s_v2_status_topic),
             "zmartify/v2/devices/%s/status", ZIC_V2_DEVICE_ID);

    config_network_t network_config = {0};
    char normalized_broker_uri[CONFIG_MQTT_URI_LEN];
    const bool has_network_config = config_get_network(&network_config) == CFG_OK;
    const bool broker_uri_normalized = has_network_config &&
        zic_normalize_broker_uri(&network_config,
                                 normalized_broker_uri,
                                 sizeof(normalized_broker_uri));
    const char *broker_uri = broker_uri_normalized
        ? normalized_broker_uri
        : (has_network_config && network_config.mqtt_broker_uri[0] != '\0')
        ? network_config.mqtt_broker_uri
        : ZIC_DEFAULT_BROKER_URI;
    const char *username = (has_network_config && network_config.mqtt_username[0] != '\0')
        ? network_config.mqtt_username
        : NULL;
    const char *password = (has_network_config && network_config.mqtt_password[0] != '\0')
        ? network_config.mqtt_password
        : NULL;
    if (broker_uri_normalized) {
        ESP_LOGW(TAG, "Normalizing malformed MQTT broker URI '%s' to '%s'",
                 network_config.mqtt_broker_uri, normalized_broker_uri);
        (void)snprintf(network_config.mqtt_broker_uri,
                       sizeof(network_config.mqtt_broker_uri),
                       "%s",
                       normalized_broker_uri);
        if (config_set_network(&network_config) == CFG_OK && config_manager_commit() == CFG_OK) {
            ESP_LOGI(TAG, "Persisted normalized MQTT broker URI");
        } else {
            ESP_LOGW(TAG, "Failed to persist normalized MQTT broker URI; using runtime fallback");
        }
    }
    ctx->mqtt_commands_authorized = zic_uri_uses_tls(broker_uri) &&
        username != NULL && password != NULL;

    mqtt_transport_config_t mqtt_cfg = {
        .broker_uri = broker_uri,
        .client_id = ZIC_V2_DEVICE_ID,
        .username = username,
        .password = password,
        .use_crt_bundle = (has_network_config && network_config.mqtt_tls_enabled) || zic_uri_uses_tls(broker_uri),
        .last_will_topic = s_v2_status_topic,
        .last_will_message = "{\"status\":\"offline\"}",
        .subscribe_topics = s_command_topics,
        .subscribe_topic_count = sizeof(s_command_topics) / sizeof(s_command_topics[0]),
        .on_message = zic_mqtt_on_message,
        .on_connected = zic_mqtt_on_connected,
        .user_ctx = ctx,
    };
    if (mqtt_transport_init(&ctx->mqtt_transport, &mqtt_cfg)) {
        mqtt_transport_start(&ctx->mqtt_transport);
    }
}

static void zic_publish_v2_outcome(zic_app_context_t *ctx,
                                   const char *correlation_id,
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
                              correlation_id,
                              NULL, zone_id)) {
        return;
    }
    int qos = severity != NULL && strcmp(severity, "critical") == 0
        ? MQTT_TRANSPORT_QOS_CRITICAL : MQTT_TRANSPORT_QOS_STATE;
    (void)mqtt_transport_publish(&ctx->mqtt_transport, s_v2_outcome_topic, payload, qos, false);
}

static bool zic_runtime_start_zone(zic_app_context_t *ctx,
                                   uint8_t zone_id,
                                   uint32_t requested_runtime_seconds)
{
    ota_state_t ota_state = ota_manager_get_state();
    if (ctx == NULL || !ctx->relay_available) {
        return false;
    }
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
    zic_publish_v2_outcome(ctx, NULL, "zone.started", "info", "completed", NULL, zone_id);
    return true;
}

static bool zic_runtime_run_relay_self_test(zic_app_context_t *ctx)
{
    if (ctx == NULL || !ctx->relay_available || !irrigation_engine_is_idle(&ctx->engine) ||
        ctx->scheduled_program_active || alarm_manager_has_lockout(&ctx->alarm_manager)) {
        return false;
    }

    const TickType_t pulse_ticks = pdMS_TO_TICKS(ZIC_RELAY_TEST_PULSE_MS);
    const TickType_t gap_ticks = pdMS_TO_TICKS(ZIC_RELAY_TEST_GAP_MS);

    ESP_LOGW(TAG, "HMI relay self-test started; each relay is pulsed in sequence");
    storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                           ZIC_LOG_AUDIT, "relay self-test started from HMI");

    relay_result_t result = relay_close_all();
    if (result != RELAY_OK) {
        ESP_LOGE(TAG, "Relay self-test aborted: close-all failed (%d)", result);
        return false;
    }

    result = relay_master_open();
    ESP_LOGI(TAG, "Relay self-test: master relay ON");
    vTaskDelay(pulse_ticks);
    if (result != RELAY_OK || relay_master_close() != RELAY_OK) {
        (void)relay_close_all();
        ESP_LOGE(TAG, "Relay self-test failed on master relay (%d)", result);
        return false;
    }
    ESP_LOGI(TAG, "Relay self-test: master relay OFF");
    vTaskDelay(gap_ticks);

    result = relay_master_open();
    if (result != RELAY_OK) {
        (void)relay_close_all();
        ESP_LOGE(TAG, "Relay self-test aborted: master relay could not be opened (%d)", result);
        return false;
    }

    for (uint8_t relay = RELAY_ZONE_FIRST; relay <= RELAY_ZONE_LAST; ++relay) {
        result = relay_zone_open(relay);
        ESP_LOGI(TAG, "Relay self-test: zone relay %u ON", (unsigned)relay);
        vTaskDelay(pulse_ticks);
        if (result != RELAY_OK || relay_zone_close(relay) != RELAY_OK) {
            (void)relay_close_all();
            ESP_LOGE(TAG, "Relay self-test failed on zone relay %u (%d)",
                     (unsigned)relay, result);
            return false;
        }
        ESP_LOGI(TAG, "Relay self-test: zone relay %u OFF", (unsigned)relay);
        vTaskDelay(gap_ticks);
    }

    result = relay_close_all();
    if (result != RELAY_OK) {
        ESP_LOGE(TAG, "Relay self-test ended with close-all failure (%d)", result);
        return false;
    }

    storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                           ZIC_LOG_AUDIT, "relay self-test completed from HMI");
    ESP_LOGI(TAG, "HMI relay self-test complete; all relays OFF");
    return true;
}

static void zic_runtime_apply_command(zic_app_context_t *ctx, const zic_runtime_command_t *cmd)
{
    switch (cmd->type) {
    case ZIC_CMD_START_ZONE: {
        if (!ctx->relay_available) {
            zic_publish_v2_outcome(ctx, cmd->command_id, "run.rejected", "warning", "rejected",
                                   "relay_unavailable", cmd->zone_id);
            break;
        }
        bool was_running = irrigation_engine_is_running(&ctx->engine);
        bool same_zone = was_running && ctx->engine.active_zone_id == cmd->zone_id;
        if (zic_runtime_start_zone(ctx, cmd->zone_id, cmd->runtime_seconds)) {
            if (!same_zone) {
                (void)snprintf(ctx->active_zone_command_id,
                               sizeof(ctx->active_zone_command_id),
                               "%s",
                               cmd->command_id);
            }
            const char *detail = same_zone ? "runtime_refreshed" : was_running ? "zone_transition" : NULL;
            zic_publish_v2_outcome(ctx, cmd->command_id, "run.started", "info", "completed", detail, cmd->zone_id);
        } else {
            zic_publish_v2_outcome(ctx, cmd->command_id, "run.rejected", "warning", "rejected",
                                   "controller_not_idle", cmd->zone_id);
        }
        break;
    }
    case ZIC_CMD_RUN_PROGRAM: {
        config_program_t program;
        uint8_t program_index = cmd->program_id - 1u;
        if (!ctx->relay_available) {
            zic_publish_v2_outcome(ctx, cmd->command_id, "run.rejected", "warning",
                                   "rejected", "relay_unavailable", 0);
            break;
        }
        if (cmd->program_id == 0u || cmd->program_id > CONFIG_MAX_PROGRAMS ||
            config_get_program(program_index, &program) != CFG_OK || !program.enabled ||
            !irrigation_engine_is_idle(&ctx->engine) || ctx->scheduled_program_active ||
            alarm_manager_has_lockout(&ctx->alarm_manager)) {
            zic_publish_v2_outcome(ctx, cmd->command_id, "run.rejected", "warning",
                                   "rejected", "program_unavailable", 0);
            break;
        }
        ctx->scheduled_program_active = true;
        ctx->scheduled_program_index = program_index;
        ctx->scheduled_next_zone_index = 0u;
        ctx->scheduled_program = program;
        (void)snprintf(ctx->scheduled_program_command_id,
                       sizeof(ctx->scheduled_program_command_id),
                       "%s",
                       cmd->command_id);
        storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                               ZIC_LOG_IRRIGATION, "program started by local command");
        zic_publish_v2_outcome(ctx, cmd->command_id, "run.started", "info",
                               "completed", NULL, 0);
        break;
    }
    case ZIC_CMD_SKIP_PROGRAM_STEP:
        if (!ctx->scheduled_program_active || irrigation_engine_is_idle(&ctx->engine)) {
            zic_publish_v2_outcome(ctx, cmd->command_id, "run.rejected", "warning",
                                   "rejected", "program_not_running", 0);
            break;
        }
        if (!irrigation_engine_stop_zone(&ctx->engine, ctx->engine.active_zone_id)) {
            zic_publish_v2_outcome(ctx, cmd->command_id, "run.rejected", "warning",
                                   "rejected", "zone_not_active", 0);
            break;
        }
        valve_diagnostics_command_close(&ctx->valve_diagnostics,
                                        (uint64_t)(esp_timer_get_time() / 1000));
        storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                               ZIC_LOG_IRRIGATION, "program zone skipped by command");
        zic_publish_v2_outcome(ctx, cmd->command_id, "run.stopped", "info",
                               "completed", "skip_next", 0);
        break;
    case ZIC_CMD_STOP_ZONE:
        ctx->scheduled_program_active = false;
        if (irrigation_engine_stop_zone(&ctx->engine, cmd->zone_id)) {
            valve_diagnostics_command_close(&ctx->valve_diagnostics,
                                            (uint64_t)(esp_timer_get_time() / 1000));
            storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                           ZIC_LOG_IRRIGATION, "zone stopped by command");
            if (ctx->active_zone_command_id[0] != '\0') {
                zic_publish_v2_outcome(ctx, ctx->active_zone_command_id, "run.completed", "info",
                                       "completed", "stopped", cmd->zone_id);
                ctx->active_zone_command_id[0] = '\0';
            }
            zic_publish_v2_outcome(ctx, cmd->command_id, "run.stopped", "info", "completed", NULL, cmd->zone_id);
        } else {
            zic_publish_v2_outcome(ctx, cmd->command_id, "run.stop_rejected", "warning", "rejected",
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
        if (ctx->active_zone_command_id[0] != '\0') {
            zic_publish_v2_outcome(ctx, ctx->active_zone_command_id, "run.completed", "info",
                                   "completed", "stop_all", 0);
            ctx->active_zone_command_id[0] = '\0';
        }
        storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                       ZIC_LOG_AUDIT, "all irrigation stopped by command");
        zic_publish_v2_outcome(ctx, cmd->command_id, "run.stopped", "info", "completed", "stop_all", 0);
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
        zic_publish_v2_outcome(ctx, cmd->command_id, "rain.delay_set", "info", "completed", NULL, 0);
        break;
    case ZIC_CMD_CLEAR_RAIN_DELAY:
        persistent_store_set_u32("rain_delay_h", 0);
        zic_controller_apply_event(&ctx->engine.controller, ZIC_EV_RAIN_DELAY_CLEAR, -1);
        storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                       ZIC_LOG_AUDIT, "rain delay cleared");
        zic_publish_v2_outcome(ctx, cmd->command_id, "rain.delay_cleared", "info", "completed", NULL, 0);
        break;
    case ZIC_CMD_REPLACE_PROGRAMS_CONFIG: {
        cfg_result_t result;
        char detail[96];

        if (!irrigation_engine_is_idle(&ctx->engine) || ctx->scheduled_program_active) {
            zic_publish_v2_outcome(ctx, cmd->command_id, "config.programs.rejected", "warning",
                                   "rejected", "controller_not_idle", 0);
            break;
        }

        result = config_replace_programs(cmd->program_sync.programs,
                                         cmd->program_sync.program_count,
                                         cmd->program_sync.timezone);
        if (result != CFG_OK) {
            const char *failure_detail = result == CFG_SAFE_MODE
                ? "config_safe_mode"
                : result == CFG_INVALID_PARAM
                ? "invalid_timezone"
                : "config_save_failed";
            zic_publish_v2_outcome(ctx, cmd->command_id, "config.programs.rejected", "warning",
                                   "rejected", failure_detail, 0);
            break;
        }

        zic_runtime_reset_scheduler_state(ctx);
    zic_scheduler_store_metadata(ctx,
                     cmd->program_sync.config_revision,
                     cmd->program_sync.program_count,
                     cmd->program_sync.schedule_count);
        zic_runtime_apply_timezone(cmd->program_sync.timezone);
        storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                               ZIC_LOG_AUDIT, "program config replaced by command");
        snprintf(detail, sizeof(detail), "program_count=%u schedule_count=%u revision=%lu",
                 (unsigned)cmd->program_sync.program_count,
                 (unsigned)cmd->program_sync.schedule_count,
                 (unsigned long)cmd->program_sync.config_revision);
        zic_publish_v2_outcome(ctx, cmd->command_id, "config.programs.applied", "info",
                               "completed", detail, 0);
        break;
    }
    case ZIC_CMD_CLEAR_PROGRAMS_CONFIG: {
        cfg_result_t result;
        char detail[96];

        if (!irrigation_engine_is_idle(&ctx->engine) || ctx->scheduled_program_active) {
            zic_publish_v2_outcome(ctx, cmd->command_id, "config.programs.rejected", "warning",
                                   "rejected", "controller_not_idle", 0);
            break;
        }

        result = config_replace_programs(NULL, 0u, NULL);
        if (result != CFG_OK) {
            const char *failure_detail = result == CFG_SAFE_MODE
                ? "config_safe_mode"
                : "config_save_failed";
            zic_publish_v2_outcome(ctx, cmd->command_id, "config.programs.rejected", "warning",
                                   "rejected", failure_detail, 0);
            break;
        }

        zic_runtime_reset_scheduler_state(ctx);
        zic_scheduler_store_metadata(ctx, cmd->program_sync.config_revision, 0u, 0u);
        storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                               ZIC_LOG_AUDIT, "program config cleared by command");
        snprintf(detail, sizeof(detail), "program_count=0 schedule_count=0 revision=%lu",
                 (unsigned long)cmd->program_sync.config_revision);
        zic_publish_v2_outcome(ctx, cmd->command_id, "config.programs.cleared", "info",
                               "completed", detail, 0);
        break;
    }
    case ZIC_CMD_UPDATE_NETWORK_CONFIG:
        if (config_set_network(&cmd->network_config) == CFG_OK && config_manager_commit() == CFG_OK) {
            storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                                   ZIC_LOG_AUDIT, "network config updated by command");
            zic_publish_v2_outcome(ctx, cmd->command_id, "config.updated", "info", "completed", "reboot_required", 0);
        } else {
            zic_publish_v2_outcome(ctx, cmd->command_id, "config.rejected", "warning", "rejected", "config_save_failed", 0);
        }
        break;
    case ZIC_CMD_INITIALIZE_SD_CARD:
        if (zic_sd_card_initialize(cmd->sd_format_if_needed) == ESP_OK) {
            storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                                   ZIC_LOG_AUDIT, "sd card initialized by command");
            zic_publish_v2_outcome(ctx, cmd->command_id, "storage.sd_card.initialized", "info", "completed", "mounted", 0);
        } else {
            storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                                   ZIC_LOG_AUDIT, "sd card initialize failed");
            zic_publish_v2_outcome(ctx, cmd->command_id, "storage.sd_card.rejected", "warning", "rejected", s_sd_card_last_error, 0);
        }
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
    case ZIC_CMD_RELAY_SELF_TEST:
        if (!ctx->relay_available) {
            zic_publish_v2_outcome(ctx, cmd->command_id, "relay.self_test", "warning",
                                   "rejected", "relay_unavailable", 0);
        } else if (zic_runtime_run_relay_self_test(ctx)) {
            zic_publish_v2_outcome(ctx, cmd->command_id, "relay.self_test", "info",
                                   "completed", NULL, 0);
        } else {
            zic_publish_v2_outcome(ctx, cmd->command_id, "relay.self_test", "warning",
                                   "rejected", "controller_not_idle", 0);
        }
        break;
    default:
        break;
    }
}

static void zic_scheduler_start_due_program(zic_app_context_t *ctx)
{
    if ((xEventGroupGetBits(s_runtime_events) & ZIC_EVENT_OTA_CONFIRMED) == 0 ||
        !ctx->relay_available || !hal_time_is_synced() || !irrigation_engine_is_idle(&ctx->engine) ||
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
        ctx->scheduled_program_command_id[0] = '\0';
        storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                       ZIC_LOG_IRRIGATION, "scheduled program started");
        ESP_LOGI(TAG, "Scheduled program %u started", program_index + 1);
        return;
    }
}

static void zic_scheduler_advance_program(zic_app_context_t *ctx)
{
    if (!ctx->relay_available) {
        ctx->scheduled_program_active = false;
        return;
    }

    if (!ctx->scheduled_program_active || !irrigation_engine_is_idle(&ctx->engine)) {
        return;
    }

    config_system_t system;
    if (config_get_system(&system) != CFG_OK || system.operational_mode != CONFIG_MODE_AUTO) {
        ctx->scheduled_program_active = false;
        return;
    }

    while (ctx->scheduled_next_zone_index < CONFIG_MAX_ZONES) {
        uint8_t next_group = 0u;
        for (uint8_t zone_index = 0; zone_index < CONFIG_MAX_ZONES; ++zone_index) {
            uint8_t group = ctx->scheduled_program.zone_group[zone_index];
            if (group == 0u) {
                group = (uint8_t)(zone_index + 1u);
            }
            if (ctx->scheduled_program.zone_runtime_min[zone_index] != 0u &&
                group > ctx->scheduled_next_zone_index &&
                (next_group == 0u || group < next_group)) {
                next_group = group;
            }
        }
        if (next_group == 0u) {
            break;
        }
        ctx->scheduled_next_zone_index = next_group;

        uint8_t started_count = 0u;
        bool group_failed = false;
        for (uint8_t zone_index = 0; zone_index < CONFIG_MAX_ZONES; ++zone_index) {
            uint8_t group = ctx->scheduled_program.zone_group[zone_index];
            if (group == 0u) {
                group = (uint8_t)(zone_index + 1u);
            }
            if (group != next_group) {
                continue;
            }

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
            if (runtime_seconds == 0u) {
                continue;
            }
            if (started_count >= IRRIGATION_MAX_CONCURRENT_ZONES ||
                !zic_runtime_start_zone(ctx, zone_index + 1u, runtime_seconds)) {
                group_failed = true;
                break;
            }
            ++started_count;
            ESP_LOGI(TAG, "Program %u running group %u zone %u for %lu seconds",
                     ctx->scheduled_program_index + 1u, next_group, zone_index + 1u,
                     (unsigned long)runtime_seconds);
        }

        if (group_failed) {
            (void)irrigation_engine_stop_all(&ctx->engine);
            ctx->scheduled_program_active = false;
            storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                                   ZIC_LOG_IRRIGATION, "scheduled program group failed");
            return;
        }
        if (started_count > 0u) {
            return;
        }
    }

    ESP_LOGI(TAG, "Scheduled program %u completed", ctx->scheduled_program_index + 1);
    storage_manager_append(&ctx->storage_manager, zic_log_timestamp(),
                           ZIC_LOG_IRRIGATION, "scheduled program completed");
    if (ctx->scheduled_program_command_id[0] != '\0') {
        zic_publish_v2_outcome(ctx,
                               ctx->scheduled_program_command_id,
                               "run.completed",
                               "info",
                               "completed",
                               NULL,
                               0);
        ctx->scheduled_program_command_id[0] = '\0';
    }
    ctx->scheduled_program_active = false;
}

static void zic_control_task(void *arg)
{
    (void)arg;

    zic_runtime_command_t *cmd = NULL;
    xEventGroupSetBits(s_runtime_events, ZIC_EVENT_ENGINE_READY);

    for (;;) {
        if (xQueueReceive(s_command_queue, &cmd, pdMS_TO_TICKS(100)) == pdTRUE && cmd != NULL) {
            xSemaphoreTake(s_ctx_lock, portMAX_DELAY);
            zic_runtime_apply_command(&s_ctx, cmd);
            xSemaphoreGive(s_ctx_lock);
            free(cmd);
            cmd = NULL;
        }

        xSemaphoreTake(s_ctx_lock, portMAX_DELAY);

        irrigation_phase_t previous_phase = s_ctx.engine.phase;
        uint8_t active_zone_ids[IRRIGATION_MAX_CONCURRENT_ZONES] = {0};
        uint8_t active_zone_count = s_ctx.engine.active_zone_count;
        for (uint8_t index = 0; index < active_zone_count; ++index) {
            active_zone_ids[index] = s_ctx.engine.active_zones[index].zone_id;
        }
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
        for (uint8_t index = 0; index < active_zone_count; ++index) {
            bool still_active = false;
            for (uint8_t current = 0; current < s_ctx.engine.active_zone_count; ++current) {
                if (active_zone_ids[index] == s_ctx.engine.active_zones[current].zone_id) {
                    still_active = true;
                    break;
                }
            }
            if (!still_active && active_zone_ids[index] != 0u) {
                storage_manager_append(&s_ctx.storage_manager, zic_log_timestamp(),
                                       ZIC_LOG_IRRIGATION, "zone completed");
                zic_publish_v2_outcome(&s_ctx, NULL, "zone.stopped", "info", "completed", NULL,
                                       active_zone_ids[index]);
            }
        }
        if (previous_phase == IRRIGATION_PHASE_MASTER_CLOSE_DELAY &&
            irrigation_engine_is_idle(&s_ctx.engine)) {
            if (!s_ctx.scheduled_program_active && s_ctx.active_zone_command_id[0] != '\0') {
                zic_publish_v2_outcome(&s_ctx, s_ctx.active_zone_command_id,
                                       "run.completed", "info", "completed", NULL, 0);
                s_ctx.active_zone_command_id[0] = '\0';
            }
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

        if (alarm_manager_has_lockout(&s_ctx.alarm_manager)) {
            if (!s_ctx.safety_fault_latched) {
                ESP_LOGE(TAG, "Critical hydraulic alarm; irrigation locked out until reboot");
                storage_manager_append(&s_ctx.storage_manager, zic_log_timestamp(), ZIC_LOG_ALARM,
                                       "critical hydraulic shutdown");
                (void)storage_manager_flush(&s_ctx.storage_manager, zic_log_timestamp());
                zic_publish_v2_outcome(&s_ctx, NULL, "irrigation.fault", "critical", "failed",
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

        zic_alarm_flush(&s_ctx.alarm_manager);

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
        (void)mqtt_transport_recover_if_stale(&s_ctx.mqtt_transport,
                                               ZIC_MQTT_STALE_RECOVERY_MS);
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
                 "Telemetry controller_state=%d active_zone=%d alarm_high_flow=%d et_daily=%.2f fault=%d mqtt=%d watersensor=%d flow_source=%d logs=%u first_log=%s",
                 (int)s_ctx.engine.controller.state,
                 (int)s_ctx.engine.controller.active_zone,
                 alarm_manager_is_active(&s_ctx.alarm_manager, ZIC_ALARM_HIGH_FLOW),
                 s_ctx.et_output.daily_et_mm,
                 (bits & ZIC_EVENT_FAULT) != 0,
                 mqtt_transport_is_connected(&s_ctx.mqtt_transport),
                 (int)watersensor_state,
                 (int)s_ctx.flow_manager.source,
                 (unsigned)storage_manager_count(&s_ctx.storage_manager),
                 csv_line);

        if (mqtt_transport_is_connected(&s_ctx.mqtt_transport)) {
            /* Zmartify v2 reported-state (schema mqtt-v2/reported-state). */
            char stamp[32];
            char next_run_at[32] = {0};
            char next_program_name[CONFIG_PROGRAM_NAME_LEN] = {0};
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
            zic_v2_storage_t storage;
            uint32_t rain_delay = 0u;
            zic_v2_scheduler_t scheduler = {
                .config_revision = s_ctx.scheduler_config_revision,
                .program_count = s_ctx.scheduler_program_count,
                .schedule_count = s_ctx.scheduler_schedule_count,
                .max_zones = CONFIG_MAX_ZONES,
                .rain_delay_active = false,
                .active_program_name = s_ctx.scheduled_program_active ? s_ctx.scheduled_program.name : NULL,
                .active_zone_id = s_ctx.engine.active_zone_id,
                .remaining_seconds = irrigation_engine_remaining_seconds(&s_ctx.engine, (uint64_t)(esp_timer_get_time() / 1000)),
                .active_zone_name = NULL,
                .next_run_at = NULL,
                .next_program_name = NULL,
                .blocked_reason = zic_scheduler_blocked_reason(&s_ctx),
            };
            if (s_ctx.engine.active_zone_id > 0u) {
                config_zone_t active_zone;
                if (config_get_zone((uint8_t)(s_ctx.engine.active_zone_id - 1u), &active_zone) == CFG_OK) {
                    scheduler.active_zone_name = active_zone.name[0] != '\0' ? active_zone.name : NULL;
                }
            }
            (void)persistent_store_get_u32("rain_delay_h", &rain_delay, 0u);
            scheduler.rain_delay_active = rain_delay > 0u;
            if (!s_ctx.scheduled_program_active && zic_scheduler_next_run_iso(
                    next_run_at, sizeof(next_run_at), next_program_name, sizeof(next_program_name))) {
                scheduler.next_run_at = next_run_at;
                scheduler.next_program_name = next_program_name;
            }
            zic_sd_card_storage_snapshot(&storage);
            const esp_app_desc_t *app_desc = esp_app_get_description();
            const char *firmware_version = app_desc != NULL ? app_desc->version : NULL;
            if (zic_v2_build_reported_state(s_v2_state_payload, sizeof(s_v2_state_payload), stamp, firmware_version,
                                            &hyd, NULL, &wx, &scheduler, &storage)) {
                mqtt_transport_publish(&s_ctx.mqtt_transport,
                                       s_v2_state_topic,
                                       s_v2_state_payload,
                                       MQTT_TRANSPORT_QOS_STATE,
                                       true);
            } else {
                ESP_LOGW(TAG, "Failed to build v2 reported state payload");
            }

        }
        xSemaphoreGive(s_ctx_lock);

        size_t diagnostics_length = diagnostics_health_to_json(
            s_diagnostics_payload, sizeof(s_diagnostics_payload));
        if (diagnostics_length > 0u &&
            mqtt_transport_is_connected(&s_ctx.mqtt_transport)) {
            mqtt_transport_publish(&s_ctx.mqtt_transport,
                                   s_diagnostics_topic,
                                   s_diagnostics_payload,
                                   1,
                                   true);
        }

        vTaskDelay(pdMS_TO_TICKS(5000));
    }
}

static bool zic_hmi_snapshot(void *context, hmi_view_model_t *view_model)
{
    zic_app_context_t *ctx = context;
    if (ctx == NULL || view_model == NULL || s_ctx_lock == NULL) {
        return false;
    }

    hmi_view_model_t snapshot = {0};
    if (xSemaphoreTake(s_ctx_lock, pdMS_TO_TICKS(100)) != pdTRUE) {
        return false;
    }

    snapshot.controller_state = (uint8_t)ctx->engine.controller.state;
    snapshot.active_zone = ctx->engine.active_zone_id;
    snapshot.remaining_seconds = irrigation_engine_remaining_seconds(
        &ctx->engine, (uint64_t)(esp_timer_get_time() / 1000));
    if (ctx->engine.active_zone_id > 0u) {
        config_zone_t active_zone;
        if (config_get_zone((uint8_t)(ctx->engine.active_zone_id - 1u), &active_zone) == CFG_OK) {
            snprintf(snapshot.active_zone_name,
                     sizeof(snapshot.active_zone_name),
                     "%s",
                     active_zone.name[0] != '\0' ? active_zone.name : "Unnamed zone");
        }
    }
    if (ctx->scheduled_program_active) {
        snprintf(snapshot.active_program_name,
                 sizeof(snapshot.active_program_name),
                 "%s",
                 ctx->scheduled_program.name[0] != '\0' ? ctx->scheduled_program.name : "Scheduled program");
    }
    snapshot.flow_lpm_x100 = ctx->flow_manager.current_lpm_x100;
    snapshot.pressure_mbar = ctx->pressure_manager.current_pressure_mbar;
    snapshot.temperature_c_x10 = (int16_t)(ctx->weather_snapshot.temperature_c * 10.0f);
    snapshot.humidity_pct = (uint8_t)ctx->weather_snapshot.humidity_pct;
    snapshot.rain_mm_x10 = (uint16_t)(ctx->weather_snapshot.rain_mm_last_24h * 10.0f);
    snapshot.et_mm_x100 = (uint16_t)(ctx->et_output.daily_et_mm * 100.0f);
    snapshot.flow_available = ctx->flow_manager.measurement_valid;
    snapshot.pressure_available = ctx->pressure_manager.measurement_valid;
    snapshot.mqtt_connected = mqtt_transport_is_connected(&ctx->mqtt_transport);
    snapshot.time_synchronized = hal_time_is_synced();
    snapshot.storage_ready = ctx->storage_ready && ctx->storage_last_write_ok;
    snapshot.config_safe_mode = config_is_safe_mode();
    snapshot.master_valve_on = relay_master_is_open();
    const esp_app_desc_t *app_desc = esp_app_get_description();
    snprintf(snapshot.firmware_version,
             sizeof(snapshot.firmware_version),
             "%s",
             app_desc != NULL ? app_desc->version : ZIC_FW_VERSION_STR);
    for (uint8_t relay = RELAY_ZONE_FIRST; relay <= RELAY_ZONE_LAST; ++relay) {
        snapshot.zone_valve_on[relay - RELAY_ZONE_FIRST] = relay_zone_is_open(relay);
    }
    for (size_t index = 0; index < ZIC_MAX_ACTIVE_ALARMS &&
         snapshot.alarm_count < HMI_MAX_VISIBLE_ALARMS; ++index) {
        const zic_alarm_t *alarm = &ctx->alarm_manager.alarms[index];
        if (alarm->state == ZIC_ALARM_STATE_CLEARED) {
            continue;
        }
        hmi_alarm_view_t *alarm_view = &snapshot.alarms[snapshot.alarm_count++];
        alarm_view->code = (uint16_t)alarm->code;
        alarm_view->severity = (uint8_t)alarm->severity;
        alarm_view->state = (uint8_t)alarm->state;
    }
    xSemaphoreGive(s_ctx_lock);

    wifi_ap_record_t ap_info = {0};
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        snapshot.wifi_connected = true;
        snapshot.wifi_rssi_dbm = ap_info.rssi;
    }

    uint32_t rain_delay = 0;
    (void)persistent_store_get_u32("rain_delay_h", &rain_delay, 0u);
    snapshot.rain_delay_hours = (uint16_t)rain_delay;
    for (uint8_t index = 0; index < CONFIG_MAX_PROGRAMS; ++index) {
        config_program_t program;
        if (config_get_program(index, &program) == CFG_OK && program.enabled) {
            ++snapshot.enabled_programs;
            snapshot.program_enabled[index] = true;
        }
    }

    *view_model = snapshot;
    return true;
}

static bool zic_hmi_dispatch(void *context, const hmi_action_t *action)
{
    zic_app_context_t *ctx = context;
    zic_runtime_command_t *command;
    if (ctx == NULL || action == NULL) {
        return false;
    }

    if (action->type == HMI_ACTION_REBOOT) {
        if (!zic_ota_image_is_confirmed() || !zic_ota_runtime_is_idle()) {
            return false;
        }
        ESP_LOGI(TAG, "Software reboot requested via HMI");
        return xTaskCreate(zic_ota_restart_task, "hmi_reboot", 2048, NULL, 5, NULL) == pdPASS;
    }

    command = calloc(1u, sizeof(*command));
    if (command == NULL) {
        return false;
    }
    strncpy(command->command_id, "hmi", sizeof(command->command_id) - 1u);
    switch (action->type) {
    case HMI_ACTION_START_ZONE:
        command->type = ZIC_CMD_START_ZONE;
        command->zone_id = action->zone_id;
        command->runtime_seconds = action->runtime_seconds;
        break;
    case HMI_ACTION_RUN_PROGRAM:
        command->type = ZIC_CMD_RUN_PROGRAM;
        command->program_id = action->program_id;
        break;
    case HMI_ACTION_STOP_ALL:
        command->type = ZIC_CMD_STOP_ALL;
        break;
    case HMI_ACTION_SET_RAIN_DELAY:
        command->type = ZIC_CMD_SET_RAIN_DELAY;
        command->rain_delay_hours = action->rain_delay_hours;
        break;
    case HMI_ACTION_CLEAR_RAIN_DELAY:
        command->type = ZIC_CMD_CLEAR_RAIN_DELAY;
        break;
    case HMI_ACTION_RELAY_SELF_TEST:
        command->type = ZIC_CMD_RELAY_SELF_TEST;
        break;
    case HMI_ACTION_ACKNOWLEDGE_ALARM:
    case HMI_ACTION_CLEAR_ALARM: {
        free(command);
        if (s_ctx_lock == NULL || xSemaphoreTake(s_ctx_lock, pdMS_TO_TICKS(100)) != pdTRUE) {
            return false;
        }
        bool accepted = action->type == HMI_ACTION_ACKNOWLEDGE_ALARM
            ? alarm_manager_acknowledge(&ctx->alarm_manager,
                                        (zic_alarm_code_t)action->alarm_code)
            : alarm_manager_manual_clear(&ctx->alarm_manager,
                                         (zic_alarm_code_t)action->alarm_code);
        if (accepted && action->type == HMI_ACTION_CLEAR_ALARM &&
            !alarm_manager_has_lockout(&ctx->alarm_manager)) {
            ctx->safety_fault_latched = false;
            (void)zic_controller_apply_event(&ctx->engine.controller,
                                             ZIC_EV_FAULT_CLEAR, -1);
            xEventGroupClearBits(s_runtime_events, ZIC_EVENT_FAULT);
        }
        zic_alarm_flush(&ctx->alarm_manager);
        xSemaphoreGive(s_ctx_lock);
        return accepted;
    }
    default:
        return false;
    }

    return zic_enqueue_command(command);
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

    s_command_queue = xQueueCreate(ZIC_CMD_QUEUE_LEN, sizeof(zic_runtime_command_t *));
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
    if (config_get_system(&system_config) != CFG_OK) {
        ESP_LOGE(TAG, "System configuration initialization failed; irrigation disabled");
        return;
    }
    s_ctx.relay_available = hal_relay_init() == HAL_OK &&
        relay_manager_init((uint8_t)system_config.max_simultaneous_zones) == RELAY_OK;
    if (!s_ctx.relay_available) {
        ESP_LOGW(TAG, "Relay initialization failed; running communications-only mode");
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
                NULL, ZIC_WATERSENSOR_TASK_PRIO, &s_watersensor_task_handle);
    }

    xSemaphoreTake(s_ctx_lock, portMAX_DELAY);
    zic_runtime_init_context(&s_ctx);
    xSemaphoreGive(s_ctx_lock);

    const hmi_board_bindings_t hmi_bindings = {
        .snapshot = zic_hmi_snapshot,
        .dispatch = zic_hmi_dispatch,
        .context = &s_ctx,
    };
    if (!hmi_board_bind(&hmi_bindings)) {
        ESP_LOGW(TAG, "HMI runtime binding unavailable");
    }

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

    xTaskCreate(zic_control_task, "zic_ctrl", ZIC_CTRL_TASK_STACK, NULL,
                ZIC_CTRL_TASK_PRIO, &s_control_task_handle);
    xTaskCreate(zic_telemetry_task, "zic_telem", ZIC_TELEM_TASK_STACK, NULL,
                ZIC_TELEM_TASK_PRIO, &s_telemetry_task_handle);
}
