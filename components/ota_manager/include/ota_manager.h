#pragma once

#include <stdbool.h>

typedef struct {
    const char *firmware_url;
    const char *cert_pem;
} ota_manager_config_t;

/**
 * @brief Trigger an OTA firmware update from the given URL.
 *
 * Blocking. On success, calls esp_restart() – this function does not return.
 * Publishes EVENT_OTA_STARTED, EVENT_OTA_COMPLETE, or EVENT_OTA_FAILED.
 */
bool ota_manager_perform(const ota_manager_config_t *config);
