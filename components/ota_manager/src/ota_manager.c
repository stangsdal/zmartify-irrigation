#include "ota_manager.h"

#include "event_bus.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_system.h"

static const char *TAG = "ota_manager";

bool ota_manager_perform(const ota_manager_config_t *config)
{
    if (config == NULL || config->firmware_url == NULL) {
        return false;
    }

    esp_http_client_config_t http_cfg = {
        .url = config->firmware_url,
        .cert_pem = config->cert_pem,
    };

    esp_https_ota_config_t ota_cfg = {
        .http_config = &http_cfg,
    };

    esp_err_t err = esp_https_ota(&ota_cfg);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "OTA failed: %s", esp_err_to_name(err));
        event_bus_publish(EVENT_OTA_FAILED, 0, EVENT_PRIORITY_HIGH, 0, NULL, 0);
        return false;
    }

    ESP_LOGI(TAG, "OTA complete – rebooting");
    event_bus_publish(EVENT_OTA_COMPLETE, 0, EVENT_PRIORITY_HIGH, 0, NULL, 0);
    esp_restart();
    return true;
}
