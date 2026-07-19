#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "ota_policy.h"

static void test_remote_transport_policy(void)
{
    assert(ota_policy_remote_url_is_trusted("https://updates.example/firmware.bin"));
    assert(!ota_policy_remote_url_is_trusted("http://updates.example/firmware.bin"));
    assert(!ota_policy_remote_url_is_trusted("https://"));
    assert(!ota_policy_remote_url_is_trusted(NULL));
}

static void test_successful_update_lifecycle(void)
{
    assert(ota_policy_transition_allowed(OTA_STATE_IDLE, OTA_STATE_DOWNLOADING));
    assert(ota_policy_transition_allowed(OTA_STATE_DOWNLOADING, OTA_STATE_VERIFYING));
    assert(ota_policy_transition_allowed(OTA_STATE_VERIFYING, OTA_STATE_PENDING_REBOOT));
    assert(ota_policy_transition_allowed(OTA_STATE_PENDING_REBOOT,
                                         OTA_STATE_PENDING_CONFIRMATION));
    assert(ota_policy_transition_allowed(OTA_STATE_PENDING_CONFIRMATION, OTA_STATE_VALID));
}

static void test_failure_and_rollback_lifecycle(void)
{
    assert(ota_policy_transition_allowed(OTA_STATE_DOWNLOADING, OTA_STATE_FAILED));
    assert(ota_policy_transition_allowed(OTA_STATE_VERIFYING, OTA_STATE_FAILED));
    assert(ota_policy_transition_allowed(OTA_STATE_PENDING_CONFIRMATION,
                                         OTA_STATE_ROLLING_BACK));
    assert(!ota_policy_transition_allowed(OTA_STATE_DOWNLOADING, OTA_STATE_VALID));
    assert(!ota_policy_transition_allowed(OTA_STATE_PENDING_CONFIRMATION,
                                          OTA_STATE_DOWNLOADING));
    assert(strcmp(ota_state_name(OTA_STATE_ROLLING_BACK), "rolling_back") == 0);
}

int main(void)
{
    test_remote_transport_policy();
    test_successful_update_lifecycle();
    test_failure_and_rollback_lifecycle();
    puts("ota policy tests passed");
    return 0;
}