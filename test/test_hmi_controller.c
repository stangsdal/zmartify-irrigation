#include <assert.h>
#include <string.h>

#include "hmi_controller.h"

typedef struct {
    hmi_action_t action;
    unsigned calls;
    bool result;
} dispatch_probe_t;

static bool dispatch_action(void *context, const hmi_action_t *action)
{
    dispatch_probe_t *probe = context;
    probe->action = *action;
    ++probe->calls;
    return probe->result;
}

static void test_navigation(void)
{
    hmi_controller_t controller;
    hmi_controller_init(&controller, dispatch_action, NULL);

    assert(hmi_controller_active_screen(&controller) == HMI_SCREEN_DASHBOARD);
    assert(hmi_controller_navigate(&controller, HMI_SCREEN_HYDRAULICS));
    assert(hmi_controller_active_screen(&controller) == HMI_SCREEN_HYDRAULICS);
    assert(!hmi_controller_navigate(&controller, HMI_SCREEN_COUNT));
    assert(hmi_controller_active_screen(&controller) == HMI_SCREEN_HYDRAULICS);
}

static void test_immediate_safety_stop(void)
{
    dispatch_probe_t probe = {.result = true};
    hmi_controller_t controller;
    hmi_controller_init(&controller, dispatch_action, &probe);
    hmi_action_t action = {.type = HMI_ACTION_STOP_ALL};

    assert(hmi_controller_request(&controller, &action) == HMI_REQUEST_DISPATCHED);
    assert(probe.calls == 1u);
    assert(probe.action.type == HMI_ACTION_STOP_ALL);
    assert(!hmi_controller_confirmation_pending(&controller, NULL));
}

static void test_confirmed_zone_start(void)
{
    dispatch_probe_t probe = {.result = true};
    hmi_controller_t controller;
    hmi_controller_init(&controller, dispatch_action, &probe);
    hmi_action_t action = {
        .type = HMI_ACTION_START_ZONE,
        .zone_id = 4u,
        .runtime_seconds = 900u,
    };

    assert(hmi_controller_request(&controller, &action) ==
           HMI_REQUEST_CONFIRMATION_REQUIRED);
    assert(probe.calls == 0u);
    hmi_action_t pending;
    assert(hmi_controller_confirmation_pending(&controller, &pending));
    assert(memcmp(&pending, &action, sizeof(action)) == 0);
    assert(hmi_controller_request(&controller, &action) == HMI_REQUEST_REJECTED);
    assert(hmi_controller_confirm(&controller, true));
    assert(probe.calls == 1u);
    assert(probe.action.zone_id == 4u);
    assert(probe.action.runtime_seconds == 900u);
}

static void test_cancel_and_validation(void)
{
    dispatch_probe_t probe = {.result = true};
    hmi_controller_t controller;
    hmi_controller_init(&controller, dispatch_action, &probe);
    hmi_action_t invalid_zone = {
        .type = HMI_ACTION_START_ZONE,
        .zone_id = 0u,
        .runtime_seconds = 300u,
    };
    hmi_action_t rain_delay = {
        .type = HMI_ACTION_SET_RAIN_DELAY,
        .rain_delay_hours = 24u,
    };

    assert(hmi_controller_request(&controller, &invalid_zone) == HMI_REQUEST_REJECTED);
    assert(hmi_controller_request(&controller, &rain_delay) ==
           HMI_REQUEST_CONFIRMATION_REQUIRED);
    assert(hmi_controller_confirm(&controller, false));
    assert(probe.calls == 0u);
    assert(!hmi_controller_confirm(&controller, true));
}

static void test_alarm_actions_are_protected(void)
{
    dispatch_probe_t probe = {.result = true};
    hmi_controller_t controller;
    hmi_controller_init(&controller, dispatch_action, &probe);
    hmi_action_t acknowledge = {
        .type = HMI_ACTION_ACKNOWLEDGE_ALARM,
        .alarm_code = 7u,
    };
    hmi_action_t clear = {
        .type = HMI_ACTION_CLEAR_ALARM,
        .alarm_code = 7u,
    };

    assert(hmi_controller_request(&controller, &acknowledge) ==
           HMI_REQUEST_CONFIRMATION_REQUIRED);
    assert(hmi_controller_confirm(&controller, true));
    assert(probe.action.type == HMI_ACTION_ACKNOWLEDGE_ALARM);
    assert(hmi_controller_request(&controller, &clear) ==
           HMI_REQUEST_CONFIRMATION_REQUIRED);
    assert(hmi_controller_confirm(&controller, true));
    assert(probe.action.type == HMI_ACTION_CLEAR_ALARM);
    assert(probe.calls == 2u);
}

static void test_program_start_is_validated_and_protected(void)
{
    dispatch_probe_t probe = {.result = true};
    hmi_controller_t controller;
    hmi_controller_init(&controller, dispatch_action, &probe);
    hmi_action_t program = {
        .type = HMI_ACTION_RUN_PROGRAM,
        .program_id = 2u,
    };

    assert(hmi_controller_request(&controller, &program) ==
           HMI_REQUEST_CONFIRMATION_REQUIRED);
    assert(probe.calls == 0u);
    assert(hmi_controller_confirm(&controller, true));
    assert(probe.action.type == HMI_ACTION_RUN_PROGRAM);
    assert(probe.action.program_id == 2u);

    program.program_id = 0u;
    assert(hmi_controller_request(&controller, &program) == HMI_REQUEST_REJECTED);
}

int main(void)
{
    test_navigation();
    test_immediate_safety_stop();
    test_confirmed_zone_start();
    test_cancel_and_validation();
    test_alarm_actions_are_protected();
    test_program_start_is_validated_and_protected();
    return 0;
}