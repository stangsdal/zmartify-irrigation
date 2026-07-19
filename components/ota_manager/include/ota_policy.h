#pragma once

#include <stdbool.h>

typedef enum {
    OTA_STATE_IDLE = 0,
    OTA_STATE_DOWNLOADING,
    OTA_STATE_VERIFYING,
    OTA_STATE_PENDING_REBOOT,
    OTA_STATE_PENDING_CONFIRMATION,
    OTA_STATE_VALID,
    OTA_STATE_FAILED,
    OTA_STATE_ROLLING_BACK
} ota_state_t;

bool ota_policy_remote_url_is_trusted(const char *url);
bool ota_policy_transition_allowed(ota_state_t from, ota_state_t to);
const char *ota_state_name(ota_state_t state);