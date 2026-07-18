#include "irrigation_engine.h"

#include "relay_manager.h"

#define MASTER_OPEN_DELAY_MS 2000u
#define MASTER_CLOSE_DELAY_MS 1000u

static void reset_runtime(irrigation_engine_t *engine)
{
    engine->phase = IRRIGATION_PHASE_IDLE;
    engine->deadline_ms = 0;
    engine->requested_runtime_seconds = 0;
    engine->active_zone_id = 0;
    engine->active_relay_index = 0;
}

static bool fail_safe(irrigation_engine_t *engine)
{
    (void)relay_close_all();
    if (engine->active_zone_id != 0) {
        (void)zone_manager_stop(&engine->zone_manager, engine->active_zone_id);
    }
    (void)zic_controller_apply_event(&engine->controller, ZIC_EV_FAULT, -1);
    engine->phase = IRRIGATION_PHASE_FAULT;
    return false;
}

void irrigation_engine_init(irrigation_engine_t *engine)
{
    if (engine == 0) {
        return;
    }

    zic_controller_init(&engine->controller);
    zone_manager_init(&engine->zone_manager);
    zic_controller_apply_event(&engine->controller, ZIC_EV_BOOT_DONE, -1);
    zic_controller_apply_event(&engine->controller, ZIC_EV_INIT_DONE, -1);
    reset_runtime(engine);
}

bool irrigation_engine_start_zone(irrigation_engine_t *engine,
                                  uint8_t zone_id,
                                  uint8_t relay_index,
                                  uint32_t runtime_seconds,
                                  uint64_t now_ms)
{
    if (engine == 0 || engine->phase != IRRIGATION_PHASE_IDLE ||
        engine->controller.state != ZIC_CTRL_IDLE ||
        relay_index < RELAY_ZONE_FIRST || relay_index > RELAY_ZONE_LAST ||
        runtime_seconds == 0) {
        return false;
    }

    if (!zone_manager_start(&engine->zone_manager, zone_id)) {
        return false;
    }

    engine->active_zone_id = zone_id;
    engine->active_relay_index = relay_index;
    engine->requested_runtime_seconds = runtime_seconds;

    if (relay_master_open() != RELAY_OK ||
        !zic_controller_apply_event(&engine->controller, ZIC_EV_START_ZONE, (int8_t)zone_id)) {
        return fail_safe(engine);
    }

    engine->phase = IRRIGATION_PHASE_MASTER_OPEN_DELAY;
    engine->deadline_ms = now_ms + MASTER_OPEN_DELAY_MS;
    return true;
}

bool irrigation_engine_stop_zone(irrigation_engine_t *engine, uint8_t zone_id)
{
    if (engine == 0 || engine->phase == IRRIGATION_PHASE_IDLE ||
        zone_id != engine->active_zone_id) {
        return false;
    }

    bool ok = true;
    if (engine->phase == IRRIGATION_PHASE_RUNNING) {
        ok = relay_zone_close(engine->active_relay_index) == RELAY_OK;
    }
    ok = relay_master_close() == RELAY_OK && ok;
    ok = zone_manager_stop(&engine->zone_manager, zone_id) && ok;
    ok = zic_controller_apply_event(&engine->controller, ZIC_EV_STOP_ZONE, (int8_t)zone_id) && ok;
    reset_runtime(engine);

    if (!ok) {
        (void)relay_close_all();
    }
    return ok;
}

bool irrigation_engine_stop_all(irrigation_engine_t *engine)
{
    if (engine == 0) {
        return false;
    }

    if (engine->phase == IRRIGATION_PHASE_IDLE) {
        return true;
    }
    return irrigation_engine_stop_zone(engine, engine->active_zone_id);
}

bool irrigation_engine_tick(irrigation_engine_t *engine, uint64_t now_ms)
{
    if (engine == 0) {
        return false;
    }
    if (engine->phase == IRRIGATION_PHASE_MASTER_OPEN_DELAY && now_ms >= engine->deadline_ms) {
        if (relay_zone_open(engine->active_relay_index) != RELAY_OK) {
            return fail_safe(engine);
        }
        engine->phase = IRRIGATION_PHASE_RUNNING;
        engine->deadline_ms = now_ms + ((uint64_t)engine->requested_runtime_seconds * 1000u);
    } else if (engine->phase == IRRIGATION_PHASE_RUNNING && now_ms >= engine->deadline_ms) {
        if (relay_zone_close(engine->active_relay_index) != RELAY_OK) {
            return fail_safe(engine);
        }
        engine->phase = IRRIGATION_PHASE_MASTER_CLOSE_DELAY;
        engine->deadline_ms = now_ms + MASTER_CLOSE_DELAY_MS;
    } else if (engine->phase == IRRIGATION_PHASE_MASTER_CLOSE_DELAY && now_ms >= engine->deadline_ms) {
        if (relay_master_close() != RELAY_OK ||
            !zone_manager_stop(&engine->zone_manager, engine->active_zone_id) ||
            !zic_controller_apply_event(&engine->controller, ZIC_EV_STOP_ZONE,
                                        (int8_t)engine->active_zone_id)) {
            return fail_safe(engine);
        }
        reset_runtime(engine);
    }
    return engine->phase != IRRIGATION_PHASE_FAULT;
}

bool irrigation_engine_is_idle(const irrigation_engine_t *engine)
{
    return engine != 0 && engine->phase == IRRIGATION_PHASE_IDLE;
}

uint32_t irrigation_engine_remaining_seconds(const irrigation_engine_t *engine, uint64_t now_ms)
{
    if (engine == 0 || engine->phase != IRRIGATION_PHASE_RUNNING || now_ms >= engine->deadline_ms) {
        return 0;
    }
    return (uint32_t)((engine->deadline_ms - now_ms + 999u) / 1000u);
}
