#include "ota_manager.h"

#include "esp_crt_bundle.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "ota_manager";
static ota_state_t s_state = OTA_STATE_IDLE;
static portMUX_TYPE s_state_lock = portMUX_INITIALIZER_UNLOCKED;

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
