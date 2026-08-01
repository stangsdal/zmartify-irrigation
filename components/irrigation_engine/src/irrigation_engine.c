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
    engine->active_zone_count = 0;
    for (uint8_t index = 0; index < IRRIGATION_MAX_CONCURRENT_ZONES; ++index) {
        engine->active_zones[index] = (irrigation_active_zone_t){0};
    }
}

static void sync_primary_zone(irrigation_engine_t *engine)
{
    if (engine->active_zone_count == 0u) {
        engine->active_zone_id = 0u;
        engine->active_relay_index = 0u;
        engine->requested_runtime_seconds = 0u;
        return;
    }

    const irrigation_active_zone_t *primary = &engine->active_zones[0];
    engine->active_zone_id = primary->zone_id;
    engine->active_relay_index = primary->relay_index;
    engine->requested_runtime_seconds = primary->requested_runtime_seconds;
}

static int find_active_zone(const irrigation_engine_t *engine, uint8_t zone_id)
{
    for (uint8_t index = 0; index < engine->active_zone_count; ++index) {
        if (engine->active_zones[index].zone_id == zone_id) {
            return (int)index;
        }
    }
    return -1;
}

static void remove_active_zone(irrigation_engine_t *engine, uint8_t index)
{
    for (; index + 1u < engine->active_zone_count; ++index) {
        engine->active_zones[index] = engine->active_zones[index + 1u];
    }
    if (engine->active_zone_count > 0u) {
        --engine->active_zone_count;
        engine->active_zones[engine->active_zone_count] = (irrigation_active_zone_t){0};
    }
    sync_primary_zone(engine);
}

static bool fail_safe(irrigation_engine_t *engine)
{
    (void)relay_close_all();
    for (uint8_t index = 0; index < engine->active_zone_count; ++index) {
        (void)zone_manager_stop(&engine->zone_manager, engine->active_zones[index].zone_id);
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
    if (engine == 0 ||
        relay_index < RELAY_ZONE_FIRST || relay_index > RELAY_ZONE_LAST ||
        runtime_seconds == 0) {
        return false;
    }

    if (engine->phase == IRRIGATION_PHASE_FAULT ||
        (engine->phase != IRRIGATION_PHASE_IDLE && engine->controller.state != ZIC_CTRL_RUNNING)) {
        return false;
    }

    int existing_index = find_active_zone(engine, zone_id);
    if (existing_index >= 0) {
        irrigation_active_zone_t *zone = &engine->active_zones[existing_index];
        zone->relay_index = relay_index;
        zone->requested_runtime_seconds = runtime_seconds;
        if (engine->phase == IRRIGATION_PHASE_RUNNING) {
            zone->deadline_ms = now_ms + ((uint64_t)runtime_seconds * 1000u);
        } else if (engine->phase == IRRIGATION_PHASE_MASTER_CLOSE_DELAY) {
            if (relay_zone_open(relay_index) != RELAY_OK) {
                return fail_safe(engine);
            }
            zone->deadline_ms = now_ms + ((uint64_t)runtime_seconds * 1000u);
            engine->phase = IRRIGATION_PHASE_RUNNING;
        }
        sync_primary_zone(engine);
        return true;
    }

    if (engine->active_zone_count >= IRRIGATION_MAX_CONCURRENT_ZONES ||
        !zone_manager_start(&engine->zone_manager, zone_id)) {
        return false;
    }

    bool was_idle = engine->phase == IRRIGATION_PHASE_IDLE;
    irrigation_active_zone_t *zone = &engine->active_zones[engine->active_zone_count++];
    *zone = (irrigation_active_zone_t){
        .requested_runtime_seconds = runtime_seconds,
        .zone_id = zone_id,
        .relay_index = relay_index,
    };
    sync_primary_zone(engine);

    if (!zic_controller_apply_event(&engine->controller, ZIC_EV_START_ZONE, (int8_t)engine->active_zone_id)) {
        return fail_safe(engine);
    }

    if (was_idle) {
        if (relay_master_open() != RELAY_OK) {
            return fail_safe(engine);
        }
        engine->phase = IRRIGATION_PHASE_MASTER_OPEN_DELAY;
        engine->deadline_ms = now_ms + MASTER_OPEN_DELAY_MS;
    } else if (engine->phase == IRRIGATION_PHASE_RUNNING ||
               engine->phase == IRRIGATION_PHASE_MASTER_CLOSE_DELAY) {
        if (relay_zone_open(relay_index) != RELAY_OK) {
            return fail_safe(engine);
        }
        zone->deadline_ms = now_ms + ((uint64_t)runtime_seconds * 1000u);
        engine->phase = IRRIGATION_PHASE_RUNNING;
    }
    return true;
}

bool irrigation_engine_stop_zone(irrigation_engine_t *engine, uint8_t zone_id)
{
    if (engine == 0 || engine->phase == IRRIGATION_PHASE_IDLE) {
        return false;
    }

    int active_index = find_active_zone(engine, zone_id);
    if (active_index < 0) {
        return false;
    }

    irrigation_active_zone_t zone = engine->active_zones[active_index];

    bool ok = true;
    if (engine->phase == IRRIGATION_PHASE_RUNNING) {
        ok = relay_zone_close(zone.relay_index) == RELAY_OK;
    }
    ok = zone_manager_stop(&engine->zone_manager, zone_id) && ok;
    remove_active_zone(engine, (uint8_t)active_index);

    if (engine->active_zone_count > 0u) {
        ok = zic_controller_apply_event(&engine->controller, ZIC_EV_START_ZONE,
                                        (int8_t)engine->active_zone_id) && ok;
    } else {
        ok = relay_master_close() == RELAY_OK && ok;
        ok = zic_controller_apply_event(&engine->controller, ZIC_EV_STOP_ZONE,
                                        (int8_t)zone_id) && ok;
        reset_runtime(engine);
    }

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

    bool ok = true;
    if (engine->phase == IRRIGATION_PHASE_RUNNING) {
        for (uint8_t index = 0; index < engine->active_zone_count; ++index) {
            ok = relay_zone_close(engine->active_zones[index].relay_index) == RELAY_OK && ok;
        }
    }
    for (uint8_t index = 0; index < engine->active_zone_count; ++index) {
        ok = zone_manager_stop(&engine->zone_manager, engine->active_zones[index].zone_id) && ok;
    }
    ok = relay_master_close() == RELAY_OK && ok;
    ok = zic_controller_apply_event(&engine->controller, ZIC_EV_STOP_ZONE,
                                    (int8_t)engine->active_zone_id) && ok;
    reset_runtime(engine);
    if (!ok) {
        (void)relay_close_all();
    }
    return ok;
}

bool irrigation_engine_tick(irrigation_engine_t *engine, uint64_t now_ms)
{
    if (engine == 0) {
        return false;
    }
    if (engine->phase == IRRIGATION_PHASE_MASTER_OPEN_DELAY && now_ms >= engine->deadline_ms) {
        for (uint8_t index = 0; index < engine->active_zone_count; ++index) {
            irrigation_active_zone_t *zone = &engine->active_zones[index];
            if (relay_zone_open(zone->relay_index) != RELAY_OK) {
                return fail_safe(engine);
            }
            zone->deadline_ms = now_ms + ((uint64_t)zone->requested_runtime_seconds * 1000u);
        }
        engine->phase = IRRIGATION_PHASE_RUNNING;
    } else if (engine->phase == IRRIGATION_PHASE_RUNNING) {
        for (uint8_t index = 0; index < engine->active_zone_count;) {
            irrigation_active_zone_t zone = engine->active_zones[index];
            if (now_ms < zone.deadline_ms) {
                ++index;
                continue;
            }
            if (relay_zone_close(zone.relay_index) != RELAY_OK ||
                !zone_manager_stop(&engine->zone_manager, zone.zone_id)) {
                return fail_safe(engine);
            }
            remove_active_zone(engine, index);
        }
        if (engine->active_zone_count == 0u) {
            engine->phase = IRRIGATION_PHASE_MASTER_CLOSE_DELAY;
            engine->deadline_ms = now_ms + MASTER_CLOSE_DELAY_MS;
        } else if (!zic_controller_apply_event(&engine->controller, ZIC_EV_START_ZONE,
                                                (int8_t)engine->active_zone_id)) {
            return fail_safe(engine);
        }
    } else if (engine->phase == IRRIGATION_PHASE_MASTER_CLOSE_DELAY && now_ms >= engine->deadline_ms) {
        if (relay_master_close() != RELAY_OK ||
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

bool irrigation_engine_is_running(const irrigation_engine_t *engine)
{
    return engine != 0 && engine->phase == IRRIGATION_PHASE_RUNNING;
}

uint32_t irrigation_engine_remaining_seconds(const irrigation_engine_t *engine, uint64_t now_ms)
{
    if (engine == 0 || engine->phase != IRRIGATION_PHASE_RUNNING) {
        return 0;
    }
    uint64_t latest_deadline = 0u;
    for (uint8_t index = 0; index < engine->active_zone_count; ++index) {
        if (engine->active_zones[index].deadline_ms > latest_deadline) {
            latest_deadline = engine->active_zones[index].deadline_ms;
        }
    }
    if (now_ms >= latest_deadline) {
        return 0;
    }
    return (uint32_t)((latest_deadline - now_ms + 999u) / 1000u);
}
