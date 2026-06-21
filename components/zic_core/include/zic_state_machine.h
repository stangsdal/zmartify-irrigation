#pragma once

#include "zic_types.h"

typedef enum {
    ZIC_EV_BOOT_DONE = 0,
    ZIC_EV_INIT_DONE,
    ZIC_EV_START_ZONE,
    ZIC_EV_STOP_ZONE,
    ZIC_EV_RAIN_DELAY_SET,
    ZIC_EV_RAIN_DELAY_CLEAR,
    ZIC_EV_FAULT,
    ZIC_EV_FAULT_CLEAR,
    ZIC_EV_EMERGENCY_STOP,
    ZIC_EV_EMERGENCY_CLEAR
} zic_controller_event_t;

void zic_controller_init(zic_controller_t *controller);
bool zic_controller_apply_event(zic_controller_t *controller, zic_controller_event_t event, int8_t zone_id);

void zic_zone_init(zic_zone_t *zone, uint8_t zone_id);
bool zic_zone_start(zic_zone_t *zone);
bool zic_zone_stop(zic_zone_t *zone);
void zic_zone_set_fault(zic_zone_t *zone);
void zic_zone_clear_fault(zic_zone_t *zone);
