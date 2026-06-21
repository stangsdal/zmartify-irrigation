#pragma once

#include <stdbool.h>
#include <stdint.h>

#define ZIC_MAX_ZONES 15

typedef enum {
    ZIC_CTRL_BOOT = 0,
    ZIC_CTRL_INIT,
    ZIC_CTRL_IDLE,
    ZIC_CTRL_RUNNING,
    ZIC_CTRL_PAUSED,
    ZIC_CTRL_RAIN_DELAY,
    ZIC_CTRL_FAULT,
    ZIC_CTRL_EMERGENCY_STOP
} zic_controller_state_t;

typedef enum {
    ZIC_ZONE_DISABLED = 0,
    ZIC_ZONE_OFF,
    ZIC_ZONE_STARTING,
    ZIC_ZONE_RUNNING,
    ZIC_ZONE_STOPPING,
    ZIC_ZONE_FAULT
} zic_zone_state_t;

typedef struct {
    uint8_t zone_id;
    zic_zone_state_t state;
    uint32_t baseline_flow_lpm_x100;
    uint32_t runtime_seconds;
} zic_zone_t;

typedef struct {
    zic_controller_state_t state;
    bool master_valve_on;
    int8_t active_zone;
    bool emergency_stop_active;
} zic_controller_t;
