#pragma once

#include <stdbool.h>
#include <stddef.h>
#include "ota_policy.h"

#define OTA_MANAGER_EDGE_URL_MAX 192
#define OTA_MANAGER_DEVICE_TOKEN_MAX 160

typedef struct {
    const char *firmware_url;
    const char *cert_pem;
} ota_manager_config_t;

void ota_manager_init(void);
ota_state_t ota_manager_get_state(void);
bool ota_manager_transition(ota_state_t state);
bool ota_manager_perform(const ota_manager_config_t *config);
bool ota_manager_configure_edge(const char *edge_url, const char *device_token);
bool ota_manager_pull_from_edge(const char *device_id, const char *current_version);
