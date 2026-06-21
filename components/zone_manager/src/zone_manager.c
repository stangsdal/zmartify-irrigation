#include "zone_manager.h"

#include "zic_state_machine.h"

void zone_manager_init(zone_manager_t *manager)
{
    if (manager == 0) {
        return;
    }

    for (uint8_t i = 0; i < ZIC_MAX_ZONES; ++i) {
        zic_zone_init(&manager->zones[i], i + 1);
    }
}

bool zone_manager_start(zone_manager_t *manager, uint8_t zone_id)
{
    if (manager == 0 || zone_id == 0 || zone_id > ZIC_MAX_ZONES) {
        return false;
    }

    zic_zone_t *zone = &manager->zones[zone_id - 1];
    if (!zic_zone_start(zone)) {
        return false;
    }

    zone->state = ZIC_ZONE_RUNNING;
    return true;
}

bool zone_manager_stop(zone_manager_t *manager, uint8_t zone_id)
{
    if (manager == 0 || zone_id == 0 || zone_id > ZIC_MAX_ZONES) {
        return false;
    }

    zic_zone_t *zone = &manager->zones[zone_id - 1];
    if (!zic_zone_stop(zone)) {
        return false;
    }

    zone->state = ZIC_ZONE_OFF;
    return true;
}

bool zone_manager_stop_all(zone_manager_t *manager)
{
    if (manager == 0) {
        return false;
    }

    bool any = false;
    for (uint8_t i = 1; i <= ZIC_MAX_ZONES; ++i) {
        if (zone_manager_stop(manager, i)) {
            any = true;
        }
    }

    return any;
}

const zic_zone_t *zone_manager_get(const zone_manager_t *manager, uint8_t zone_id)
{
    if (manager == 0 || zone_id == 0 || zone_id > ZIC_MAX_ZONES) {
        return 0;
    }

    return &manager->zones[zone_id - 1];
}
