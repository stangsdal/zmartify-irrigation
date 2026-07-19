#include "ota_policy.h"

#include <string.h>

bool ota_policy_remote_url_is_trusted(const char *url)
{
    return url != NULL && strncmp(url, "https://", 8) == 0 && url[8] != '\0';
}

bool ota_policy_transition_allowed(ota_state_t from, ota_state_t to)
{
    switch (from) {
    case OTA_STATE_IDLE:
    case OTA_STATE_VALID:
    case OTA_STATE_FAILED:
        return to == OTA_STATE_DOWNLOADING || to == OTA_STATE_PENDING_CONFIRMATION;
    case OTA_STATE_DOWNLOADING:
        return to == OTA_STATE_VERIFYING || to == OTA_STATE_FAILED;
    case OTA_STATE_VERIFYING:
        return to == OTA_STATE_PENDING_REBOOT || to == OTA_STATE_FAILED;
    case OTA_STATE_PENDING_REBOOT:
        return to == OTA_STATE_PENDING_CONFIRMATION || to == OTA_STATE_FAILED;
    case OTA_STATE_PENDING_CONFIRMATION:
        return to == OTA_STATE_VALID || to == OTA_STATE_ROLLING_BACK;
    case OTA_STATE_ROLLING_BACK:
        return to == OTA_STATE_FAILED;
    default:
        return false;
    }
}

const char *ota_state_name(ota_state_t state)
{
    static const char *const names[] = {
        "idle",
        "downloading",
        "verifying",
        "pending_reboot",
        "pending_confirmation",
        "valid",
        "failed",
        "rolling_back",
    };

    return state >= OTA_STATE_IDLE && state <= OTA_STATE_ROLLING_BACK
        ? names[state]
        : "unknown";
}