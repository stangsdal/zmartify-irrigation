#include "ota_manager.h"

#include "esp_crt_bundle.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_ota_ops.h"
#include "cJSON.h"
#include "mbedtls/md.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include <stdio.h>
#include <string.h>

static const char *TAG = "ota_manager";
static ota_state_t s_state = OTA_STATE_IDLE;
static portMUX_TYPE s_state_lock = portMUX_INITIALIZER_UNLOCKED;

typedef struct {
    uint32_t version;
    char edge_url[OTA_MANAGER_EDGE_URL_MAX];
    char device_token[OTA_MANAGER_DEVICE_TOKEN_MAX];
} ota_edge_config_t;

static bool ota_load_edge_config(ota_edge_config_t *config)
{
    nvs_handle_t handle;
    size_t size = sizeof(*config);
    if (config == NULL || nvs_open("ota_pull", NVS_READONLY, &handle) != ESP_OK) return false;
    esp_err_t err = nvs_get_blob(handle, "cfg", config, &size);
    nvs_close(handle);
    return err == ESP_OK && size == sizeof(*config) && config->version == 1U;
}

static esp_err_t ota_get_json(const char *url, const char *token, char *body, size_t body_size)
{
    esp_http_client_config_t config = {.url = url, .crt_bundle_attach = esp_crt_bundle_attach, .timeout_ms = 15000};
    esp_http_client_handle_t client = esp_http_client_init(&config);
    if (client == NULL) return ESP_ERR_NO_MEM;
    char authorization[OTA_MANAGER_DEVICE_TOKEN_MAX + 8];
    snprintf(authorization, sizeof(authorization), "Bearer %s", token);
    esp_http_client_set_header(client, "Authorization", authorization);
    esp_err_t err = esp_http_client_open(client, 0);
    if (err == ESP_OK) {
        int length = esp_http_client_fetch_headers(client);
        if (esp_http_client_get_status_code(client) != 200 || length < 0 || (size_t)length >= body_size) err = ESP_FAIL;
        if (err == ESP_OK) {
            int read = esp_http_client_read_response(client, body, body_size - 1U);
            if (read < 0) err = ESP_FAIL; else body[read] = '\0';
        }
    }
    esp_http_client_close(client);
    esp_http_client_cleanup(client);
    return err;
}

bool ota_manager_configure_edge(const char *edge_url, const char *device_token)
{
    if (!ota_policy_remote_url_is_trusted(edge_url) || device_token == NULL || device_token[0] == '\0' ||
        strlen(edge_url) >= OTA_MANAGER_EDGE_URL_MAX || strlen(device_token) >= OTA_MANAGER_DEVICE_TOKEN_MAX) return false;
    ota_edge_config_t config = {.version = 1U};
    snprintf(config.edge_url, sizeof(config.edge_url), "%s", edge_url);
    snprintf(config.device_token, sizeof(config.device_token), "%s", device_token);
    nvs_handle_t handle;
    if (nvs_open("ota_pull", NVS_READWRITE, &handle) != ESP_OK) return false;
    esp_err_t err = nvs_set_blob(handle, "cfg", &config, sizeof(config));
    if (err == ESP_OK) err = nvs_commit(handle);
    nvs_close(handle);
    return err == ESP_OK;
}

bool ota_manager_pull_from_edge(const char *device_id, const char *current_version)
{
    ota_edge_config_t config;
    char poll_url[384];
    char poll_body[1024];
    if (device_id == NULL || current_version == NULL || !ota_load_edge_config(&config)) return false;
    snprintf(poll_url, sizeof(poll_url), "%s/api/v2/devices/%s/ota/poll?current_version=%s", config.edge_url, device_id, current_version);
    if (ota_get_json(poll_url, config.device_token, poll_body, sizeof(poll_body)) != ESP_OK) return false;
    cJSON *poll = cJSON_Parse(poll_body);
    cJSON *available = cJSON_GetObjectItemCaseSensitive(poll, "update_available");
    cJSON *url = cJSON_GetObjectItemCaseSensitive(poll, "download_url");
    cJSON *sha = cJSON_GetObjectItemCaseSensitive(poll, "sha256");
    cJSON *size = cJSON_GetObjectItemCaseSensitive(poll, "size_bytes");
    bool valid = cJSON_IsTrue(available) && cJSON_IsString(url) && cJSON_IsString(sha) &&
        strlen(sha->valuestring) == 64U && cJSON_IsNumber(size) && size->valuedouble > 0;
    if (!valid || !ota_policy_remote_url_is_trusted(url->valuestring) || !ota_manager_transition(OTA_STATE_DOWNLOADING)) {
        cJSON_Delete(poll); return false;
    }
    esp_http_client_config_t http_config = {.url = url->valuestring, .crt_bundle_attach = esp_crt_bundle_attach, .timeout_ms = 30000};
    esp_http_client_handle_t client = esp_http_client_init(&http_config);
    char authorization[OTA_MANAGER_DEVICE_TOKEN_MAX + 8];
    snprintf(authorization, sizeof(authorization), "Bearer %s", config.device_token);
    esp_http_client_set_header(client, "Authorization", authorization);
    esp_err_t err = esp_http_client_open(client, 0);
    const esp_partition_t *partition = esp_ota_get_next_update_partition(NULL);
    esp_ota_handle_t handle = 0;
    uint32_t written = 0;
    mbedtls_md_context_t digest;
    mbedtls_md_init(&digest);
    if (err == ESP_OK && esp_http_client_fetch_headers(client) >= 0 && esp_http_client_get_status_code(client) == 200 && partition != NULL)
        err = esp_ota_begin(partition, OTA_SIZE_UNKNOWN, &handle);
    if (err == ESP_OK) err = mbedtls_md_setup(&digest, mbedtls_md_info_from_type(MBEDTLS_MD_SHA256), 0);
    if (err == ESP_OK) err = mbedtls_md_starts(&digest);
    uint8_t buffer[1024];
    while (err == ESP_OK) {
        int read = esp_http_client_read(client, (char *)buffer, sizeof(buffer));
        if (read < 0) { err = ESP_FAIL; break; }
        if (read == 0) break;
        if ((err = esp_ota_write(handle, buffer, (size_t)read)) != ESP_OK || mbedtls_md_update(&digest, buffer, (size_t)read) != 0) { err = ESP_FAIL; break; }
        written += (uint32_t)read;
    }
    uint8_t digest_bytes[32]; char digest_hex[65] = {0};
    if (err == ESP_OK && (mbedtls_md_finish(&digest, digest_bytes) != 0 || written != (uint32_t)size->valuedouble)) err = ESP_FAIL;
    for (size_t index = 0; index < sizeof(digest_bytes); ++index) snprintf(&digest_hex[index * 2U], 3U, "%02x", digest_bytes[index]);
    if (err == ESP_OK && strcmp(digest_hex, sha->valuestring) != 0) err = ESP_ERR_INVALID_CRC;
    mbedtls_md_free(&digest);
    if (err == ESP_OK) err = esp_ota_end(handle);
    if (err == ESP_OK) err = esp_ota_set_boot_partition(partition);
    if (err != ESP_OK && handle != 0) (void)esp_ota_abort(handle);
    esp_http_client_close(client); esp_http_client_cleanup(client); cJSON_Delete(poll);
    if (err != ESP_OK) { (void)ota_manager_transition(OTA_STATE_FAILED); return false; }
    return ota_manager_transition(OTA_STATE_VERIFYING) && ota_manager_transition(OTA_STATE_PENDING_REBOOT);
}

void ota_manager_init(void)
{
    portENTER_CRITICAL(&s_state_lock);
    s_state = OTA_STATE_IDLE;
    portEXIT_CRITICAL(&s_state_lock);
}

ota_state_t ota_manager_get_state(void)
{
    portENTER_CRITICAL(&s_state_lock);
    ota_state_t state = s_state;
    portEXIT_CRITICAL(&s_state_lock);
    return state;
}

bool ota_manager_transition(ota_state_t state)
{
    portENTER_CRITICAL(&s_state_lock);
    ota_state_t previous = s_state;
    bool allowed = ota_policy_transition_allowed(previous, state);
    if (allowed) {
        s_state = state;
    }
    portEXIT_CRITICAL(&s_state_lock);

    if (!allowed) {
        ESP_LOGW(TAG, "Rejected OTA state transition %s -> %s",
                 ota_state_name(previous), ota_state_name(state));
        return false;
    }

    ESP_LOGI(TAG, "OTA state %s -> %s", ota_state_name(previous), ota_state_name(state));
    return true;
}

bool ota_manager_perform(const ota_manager_config_t *config)
{
    if (config == NULL || !ota_policy_remote_url_is_trusted(config->firmware_url)) {
        ESP_LOGE(TAG, "Remote OTA requires an HTTPS URL");
        return false;
    }

    if (!ota_manager_transition(OTA_STATE_DOWNLOADING)) {
        return false;
    }

    esp_http_client_config_t http_cfg = {
        .url = config->firmware_url,
        .cert_pem = config->cert_pem,
        .crt_bundle_attach = config->cert_pem == NULL ? esp_crt_bundle_attach : NULL,
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    esp_err_t err = esp_https_ota(&ota_cfg);
    if (err != ESP_OK) {
        (void)ota_manager_transition(OTA_STATE_FAILED);
        ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(err));
        return false;
    }

    if (!ota_manager_transition(OTA_STATE_VERIFYING) ||
        !ota_manager_transition(OTA_STATE_PENDING_REBOOT)) {
        return false;
    }

    ESP_LOGI(TAG, "Signed OTA image accepted and ready to reboot");
    return true;
}
