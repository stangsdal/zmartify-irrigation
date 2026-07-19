#include "hmi_controller.h"

#include <string.h>

static bool action_is_valid(const hmi_action_t *action)
{
    if (action == NULL) {
        return false;
    }

    switch (action->type) {
    case HMI_ACTION_START_ZONE:
        return action->zone_id >= 1u && action->zone_id <= 15u &&
            action->runtime_seconds >= 1u && action->runtime_seconds <= 7200u;
    case HMI_ACTION_RUN_PROGRAM:
        return action->program_id >= 1u && action->program_id <= 8u;
    case HMI_ACTION_STOP_ALL:
    case HMI_ACTION_CLEAR_RAIN_DELAY:
        return true;
    case HMI_ACTION_SET_RAIN_DELAY:
        return action->rain_delay_hours >= 1u && action->rain_delay_hours <= 8760u;
    case HMI_ACTION_ACKNOWLEDGE_ALARM:
    case HMI_ACTION_CLEAR_ALARM:
        return action->alarm_code != 0u;
    default:
        return false;
    }
}

static bool action_requires_confirmation(hmi_action_type_t type)
{
    return type != HMI_ACTION_STOP_ALL;
}

void hmi_controller_init(hmi_controller_t *controller,
                         hmi_dispatch_fn dispatch,
                         void *dispatch_context)
{
    if (controller == NULL) {
        return;
    }

    *controller = (hmi_controller_t){
        .active_screen = HMI_SCREEN_DASHBOARD,
        .dispatch = dispatch,
        .dispatch_context = dispatch_context,
    };
}

bool hmi_controller_navigate(hmi_controller_t *controller, hmi_screen_t screen)
{
    if (controller == NULL || screen < HMI_SCREEN_DASHBOARD || screen >= HMI_SCREEN_COUNT) {
        return false;
    }

    controller->active_screen = screen;
    return true;
}

hmi_screen_t hmi_controller_active_screen(const hmi_controller_t *controller)
{
    return controller != NULL ? controller->active_screen : HMI_SCREEN_DASHBOARD;
}

hmi_request_result_t hmi_controller_request(hmi_controller_t *controller,
                                            const hmi_action_t *action)
{
    if (controller == NULL || controller->dispatch == NULL ||
        controller->confirmation_pending || !action_is_valid(action)) {
        return HMI_REQUEST_REJECTED;
    }

    if (!action_requires_confirmation(action->type)) {
        return controller->dispatch(controller->dispatch_context, action)
            ? HMI_REQUEST_DISPATCHED
            : HMI_REQUEST_REJECTED;
    }

    controller->pending_action = *action;
    controller->confirmation_pending = true;
    return HMI_REQUEST_CONFIRMATION_REQUIRED;
}

bool hmi_controller_confirm(hmi_controller_t *controller, bool accepted)
{
    if (controller == NULL || !controller->confirmation_pending) {
        return false;
    }

    hmi_action_t action = controller->pending_action;
    memset(&controller->pending_action, 0, sizeof(controller->pending_action));
    controller->confirmation_pending = false;
    return !accepted || controller->dispatch(controller->dispatch_context, &action);
}

bool hmi_controller_confirmation_pending(const hmi_controller_t *controller,
                                         hmi_action_t *action_out)
{
    if (controller == NULL || !controller->confirmation_pending) {
        return false;
    }

    if (action_out != NULL) {
        *action_out = controller->pending_action;
    }
    return true;
}

const char *hmi_controller_confirmation_text(const hmi_action_t *action)
{
    if (action == NULL) {
        return "Confirm action";
    }

    switch (action->type) {
    case HMI_ACTION_START_ZONE:
        return "Start selected zone?";
    case HMI_ACTION_RUN_PROGRAM:
        return "Run selected irrigation program?";
    case HMI_ACTION_SET_RAIN_DELAY:
        return "Apply rain delay and stop irrigation?";
    case HMI_ACTION_CLEAR_RAIN_DELAY:
        return "Clear rain delay?";
    case HMI_ACTION_ACKNOWLEDGE_ALARM:
        return "Acknowledge this alarm?";
    case HMI_ACTION_CLEAR_ALARM:
        return "Clear resolved alarm and release lockout?";
    case HMI_ACTION_STOP_ALL:
    default:
        return "Confirm action";
    }
}