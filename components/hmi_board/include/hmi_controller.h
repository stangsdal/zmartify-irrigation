#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define HMI_MAX_VISIBLE_ALARMS 8u

typedef enum {
    HMI_SCREEN_DASHBOARD = 0,
    HMI_SCREEN_IRRIGATION,
    HMI_SCREEN_WEATHER,
    HMI_SCREEN_HYDRAULICS,
    HMI_SCREEN_SETTINGS,
    HMI_SCREEN_COUNT
} hmi_screen_t;

typedef enum {
    HMI_ACTION_START_ZONE = 0,
    HMI_ACTION_RUN_PROGRAM,
    HMI_ACTION_STOP_ALL,
    HMI_ACTION_SET_RAIN_DELAY,
    HMI_ACTION_CLEAR_RAIN_DELAY,
    HMI_ACTION_ACKNOWLEDGE_ALARM,
    HMI_ACTION_CLEAR_ALARM
} hmi_action_type_t;

typedef struct {
    hmi_action_type_t type;
    uint8_t zone_id;
    uint8_t program_id;
    uint32_t runtime_seconds;
    uint16_t rain_delay_hours;
    uint16_t alarm_code;
} hmi_action_t;

typedef struct {
    uint16_t code;
    uint8_t severity;
    uint8_t state;
} hmi_alarm_view_t;

typedef struct {
    uint8_t controller_state;
    uint8_t active_zone;
    uint32_t remaining_seconds;
    uint32_t flow_lpm_x100;
    uint32_t pressure_mbar;
    int16_t temperature_c_x10;
    uint8_t humidity_pct;
    uint16_t rain_mm_x10;
    uint16_t et_mm_x100;
    uint16_t rain_delay_hours;
    uint8_t enabled_programs;
    bool program_enabled[8];
    bool flow_available;
    bool pressure_available;
    bool mqtt_connected;
    bool time_synchronized;
    bool storage_ready;
    bool config_safe_mode;
    hmi_alarm_view_t alarms[HMI_MAX_VISIBLE_ALARMS];
    uint8_t alarm_count;
} hmi_view_model_t;

typedef bool (*hmi_dispatch_fn)(void *context, const hmi_action_t *action);

typedef enum {
    HMI_REQUEST_REJECTED = 0,
    HMI_REQUEST_DISPATCHED,
    HMI_REQUEST_CONFIRMATION_REQUIRED
} hmi_request_result_t;

typedef struct {
    hmi_screen_t active_screen;
    hmi_dispatch_fn dispatch;
    void *dispatch_context;
    hmi_action_t pending_action;
    bool confirmation_pending;
} hmi_controller_t;

void hmi_controller_init(hmi_controller_t *controller,
                         hmi_dispatch_fn dispatch,
                         void *dispatch_context);
bool hmi_controller_navigate(hmi_controller_t *controller, hmi_screen_t screen);
hmi_screen_t hmi_controller_active_screen(const hmi_controller_t *controller);
hmi_request_result_t hmi_controller_request(hmi_controller_t *controller,
                                            const hmi_action_t *action);
bool hmi_controller_confirm(hmi_controller_t *controller, bool accepted);
bool hmi_controller_confirmation_pending(const hmi_controller_t *controller,
                                         hmi_action_t *action_out);
const char *hmi_controller_confirmation_text(const hmi_action_t *action);