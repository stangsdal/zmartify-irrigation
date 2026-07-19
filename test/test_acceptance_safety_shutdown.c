#include <assert.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "alarm_manager.h"
#include "irrigation_engine.h"
#include "pressure_manager.h"
#include "relay_manager.h"
#include "storage_manager.h"

typedef enum {
    RELAY_CALL_MASTER_OPEN,
    RELAY_CALL_ZONE_OPEN,
    RELAY_CALL_ZONE_CLOSE,
    RELAY_CALL_MASTER_CLOSE,
} relay_call_t;

static relay_call_t relay_calls[8];
static size_t relay_call_count;
static uint8_t persisted_log[4096];
static size_t persisted_log_length;

relay_result_t relay_master_open(void)
{
    relay_calls[relay_call_count++] = RELAY_CALL_MASTER_OPEN;
    return RELAY_OK;
}

relay_result_t relay_master_close(void)
{
    relay_calls[relay_call_count++] = RELAY_CALL_MASTER_CLOSE;
    return RELAY_OK;
}

relay_result_t relay_zone_open(uint8_t relay_index)
{
    assert(relay_index == 1);
    relay_calls[relay_call_count++] = RELAY_CALL_ZONE_OPEN;
    return RELAY_OK;
}

relay_result_t relay_zone_close(uint8_t relay_index)
{
    assert(relay_index == 1);
    relay_calls[relay_call_count++] = RELAY_CALL_ZONE_CLOSE;
    return RELAY_OK;
}

relay_result_t relay_close_all(void)
{
    return RELAY_OK;
}

static bool save_log(void *context, const void *data, size_t length)
{
    (void)context;
    assert(length <= sizeof(persisted_log));
    memcpy(persisted_log, data, length);
    persisted_log_length = length;
    return true;
}

static bool load_log(void *context, void *data, size_t *length)
{
    (void)context;
    if (persisted_log_length == 0 || *length < persisted_log_length) {
        return false;
    }
    memcpy(data, persisted_log, persisted_log_length);
    *length = persisted_log_length;
    return true;
}

static void test_pressure_collapse_stops_and_persists(void)
{
    irrigation_engine_t irrigation;
    alarm_manager_t alarms;
    pressure_manager_t pressure;
    zic_log_entry_t log_entries[8];
    storage_manager_t storage;
    const pressure_supervision_config_t supervision = {
        .low_pressure_mbar = 2000,
        .high_pressure_mbar = 7000,
        .critical_duration_ms = 5000,
    };

    relay_call_count = 0;
    persisted_log_length = 0;
    irrigation_engine_init(&irrigation);
    alarm_manager_init(&alarms);
    pressure_manager_init(&pressure, 2000, 7000);
    assert(storage_manager_init(&storage, log_entries, 8));
    storage_manager_set_persistence(&storage, load_log, save_log, NULL);

    assert(irrigation_engine_start_zone(&irrigation, 1, 1, 600, 0));
    assert(irrigation_engine_tick(&irrigation, 2000));
    assert(irrigation.phase == IRRIGATION_PHASE_RUNNING);

    assert(pressure_manager_supervise(&pressure, &supervision, true, 1200, true,
                                      3000, &alarms));
    assert(!alarm_manager_has_severity(&alarms, ZIC_ALARM_CRITICAL));
    assert(pressure_manager_supervise(&pressure, &supervision, true, 1200, true,
                                      8000, &alarms));
    assert(alarm_manager_is_active(&alarms, ZIC_ALARM_PRESSURE_COLLAPSE));
    assert(alarm_manager_has_severity(&alarms, ZIC_ALARM_CRITICAL));

    assert(irrigation_engine_stop_all(&irrigation));
    storage_manager_append(&storage, 123456, ZIC_LOG_ALARM,
                           "critical pressure collapse; irrigation stopped");
    assert(storage_manager_flush(&storage, 123456));

    assert(irrigation_engine_is_idle(&irrigation));
    assert(relay_call_count == 4);
    assert(relay_calls[0] == RELAY_CALL_MASTER_OPEN);
    assert(relay_calls[1] == RELAY_CALL_ZONE_OPEN);
    assert(relay_calls[2] == RELAY_CALL_ZONE_CLOSE);
    assert(relay_calls[3] == RELAY_CALL_MASTER_CLOSE);

    zic_log_entry_t restored_entries[8];
    storage_manager_t restored;
    assert(storage_manager_init(&restored, restored_entries, 8));
    storage_manager_set_persistence(&restored, load_log, save_log, NULL);
    assert(storage_manager_restore(&restored));
    assert(storage_manager_count(&restored) == 1);
    zic_log_entry_t restored_alarm;
    assert(storage_manager_get(&restored, 0, &restored_alarm));
    assert(restored_alarm.type == ZIC_LOG_ALARM);
    assert(strstr(restored_alarm.message, "pressure collapse") != NULL);
}

int main(void)
{
    test_pressure_collapse_stops_and_persists();
    return 0;
}