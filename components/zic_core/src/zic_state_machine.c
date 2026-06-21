#include "zic_state_machine.h"

void zic_controller_init(zic_controller_t *controller)
{
    if (controller == 0) {
        return;
    }

    controller->state = ZIC_CTRL_BOOT;
    controller->master_valve_on = false;
    controller->active_zone = -1;
    controller->emergency_stop_active = false;
}

bool zic_controller_apply_event(zic_controller_t *controller, zic_controller_event_t event, int8_t zone_id)
{
    if (controller == 0) {
        return false;
    }

    if (controller->emergency_stop_active && event != ZIC_EV_EMERGENCY_CLEAR) {
        return false;
    }

    switch (event) {
    case ZIC_EV_BOOT_DONE:
        if (controller->state == ZIC_CTRL_BOOT) {
            controller->state = ZIC_CTRL_INIT;
            return true;
        }
        return false;

    case ZIC_EV_INIT_DONE:
        if (controller->state == ZIC_CTRL_INIT) {
            controller->state = ZIC_CTRL_IDLE;
            return true;
        }
        return false;

    case ZIC_EV_START_ZONE:
        if (controller->state == ZIC_CTRL_IDLE || controller->state == ZIC_CTRL_RUNNING) {
            controller->state = ZIC_CTRL_RUNNING;
            controller->master_valve_on = true;
            controller->active_zone = zone_id;
            return true;
        }
        return false;

    case ZIC_EV_STOP_ZONE:
        if (controller->state == ZIC_CTRL_RUNNING) {
            controller->active_zone = -1;
            controller->master_valve_on = false;
            controller->state = ZIC_CTRL_IDLE;
            return true;
        }
        return false;

    case ZIC_EV_RAIN_DELAY_SET:
        if (controller->state == ZIC_CTRL_IDLE || controller->state == ZIC_CTRL_RUNNING) {
            controller->state = ZIC_CTRL_RAIN_DELAY;
            controller->active_zone = -1;
            controller->master_valve_on = false;
            return true;
        }
        return false;

    case ZIC_EV_RAIN_DELAY_CLEAR:
        if (controller->state == ZIC_CTRL_RAIN_DELAY) {
            controller->state = ZIC_CTRL_IDLE;
            return true;
        }
        return false;

    case ZIC_EV_FAULT:
        controller->state = ZIC_CTRL_FAULT;
        controller->active_zone = -1;
        controller->master_valve_on = false;
        return true;

    case ZIC_EV_FAULT_CLEAR:
        if (controller->state == ZIC_CTRL_FAULT) {
            controller->state = ZIC_CTRL_IDLE;
            return true;
        }
        return false;

    case ZIC_EV_EMERGENCY_STOP:
        controller->state = ZIC_CTRL_EMERGENCY_STOP;
        controller->master_valve_on = false;
        controller->active_zone = -1;
        controller->emergency_stop_active = true;
        return true;

    case ZIC_EV_EMERGENCY_CLEAR:
        if (controller->state == ZIC_CTRL_EMERGENCY_STOP) {
            controller->emergency_stop_active = false;
            controller->state = ZIC_CTRL_IDLE;
            return true;
        }
        return false;

    default:
        return false;
    }
}

void zic_zone_init(zic_zone_t *zone, uint8_t zone_id)
{
    if (zone == 0) {
        return;
    }

    zone->zone_id = zone_id;
    zone->state = ZIC_ZONE_OFF;
    zone->baseline_flow_lpm_x100 = 0;
    zone->runtime_seconds = 0;
}

bool zic_zone_start(zic_zone_t *zone)
{
    if (zone == 0) {
        return false;
    }

    if (zone->state == ZIC_ZONE_OFF) {
        zone->state = ZIC_ZONE_STARTING;
        return true;
    }

    return false;
}

bool zic_zone_stop(zic_zone_t *zone)
{
    if (zone == 0) {
        return false;
    }

    if (zone->state == ZIC_ZONE_RUNNING || zone->state == ZIC_ZONE_STARTING) {
        zone->state = ZIC_ZONE_STOPPING;
        return true;
    }

    return false;
}

void zic_zone_set_fault(zic_zone_t *zone)
{
    if (zone == 0) {
        return;
    }

    zone->state = ZIC_ZONE_FAULT;
}

void zic_zone_clear_fault(zic_zone_t *zone)
{
    if (zone == 0) {
        return;
    }

    if (zone->state == ZIC_ZONE_FAULT) {
        zone->state = ZIC_ZONE_OFF;
    }
}
