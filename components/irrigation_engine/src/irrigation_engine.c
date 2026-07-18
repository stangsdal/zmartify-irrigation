#include "irrigation_engine.h"

void irrigation_engine_init(irrigation_engine_t *engine)
{
    if (engine == 0) {
        return;
    }

    zic_controller_init(&engine->controller);
    zone_manager_init(&engine->zone_manager);
    zic_controller_apply_event(&engine->controller, ZIC_EV_BOOT_DONE, -1);
    zic_controller_apply_event(&engine->controller, ZIC_EV_INIT_DONE, -1);
}

bool irrigation_engine_start_zone(irrigation_engine_t *engine, uint8_t zone_id, uint32_t runtime_seconds)
{
    (void)runtime_seconds;

    if (engine == 0) {
        return false;
    }

    if (!zone_manager_start(&engine->zone_manager, zone_id)) {
        return false;
    }

    return zic_controller_apply_event(&engine->controller, ZIC_EV_START_ZONE, (int8_t)zone_id);
}

bool irrigation_engine_stop_zone(irrigation_engine_t *engine, uint8_t zone_id)
{
    if (engine == 0) {
        return false;
    }

    if (!zone_manager_stop(&engine->zone_manager, zone_id)) {
        return false;
    }

    return zic_controller_apply_event(&engine->controller, ZIC_EV_STOP_ZONE, (int8_t)zone_id);
}

bool irrigation_engine_stop_all(irrigation_engine_t *engine)
{
    if (engine == 0) {
        return false;
    }

    zone_manager_stop_all(&engine->zone_manager);
    return zic_controller_apply_event(&engine->controller, ZIC_EV_STOP_ZONE, -1);
}
