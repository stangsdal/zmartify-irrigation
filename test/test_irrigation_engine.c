#include <assert.h>
#include <stddef.h>

#include "irrigation_engine.h"
#include "relay_manager.h"

typedef enum {
    CALL_MASTER_OPEN,
    CALL_ZONE_OPEN,
    CALL_ZONE_CLOSE,
    CALL_MASTER_CLOSE,
    CALL_CLOSE_ALL,
} relay_call_t;

typedef struct {
    relay_call_t call;
    uint8_t relay;
} relay_call_record_t;

static relay_call_record_t calls[12];
static size_t call_count;

relay_result_t relay_master_open(void)
{
    calls[call_count++] = (relay_call_record_t){CALL_MASTER_OPEN, 0};
    return RELAY_OK;
}

relay_result_t relay_master_close(void)
{
    calls[call_count++] = (relay_call_record_t){CALL_MASTER_CLOSE, 0};
    return RELAY_OK;
}

relay_result_t relay_zone_open(uint8_t relay)
{
    calls[call_count++] = (relay_call_record_t){CALL_ZONE_OPEN, relay};
    return RELAY_OK;
}

relay_result_t relay_zone_close(uint8_t relay)
{
    calls[call_count++] = (relay_call_record_t){CALL_ZONE_CLOSE, relay};
    return RELAY_OK;
}

relay_result_t relay_close_all(void)
{
    calls[call_count++] = (relay_call_record_t){CALL_CLOSE_ALL, 0};
    return RELAY_OK;
}

static void test_timed_relay_sequence(void)
{
    irrigation_engine_t engine;
    call_count = 0;
    irrigation_engine_init(&engine);

    assert(irrigation_engine_start_zone(&engine, 2, 3, 5, 1000));
    assert(call_count == 1 && calls[0].call == CALL_MASTER_OPEN);
    assert(irrigation_engine_tick(&engine, 2999));
    assert(call_count == 1);

    assert(irrigation_engine_tick(&engine, 3000));
    assert(call_count == 2 && calls[1].call == CALL_ZONE_OPEN && calls[1].relay == 3);
    assert(irrigation_engine_remaining_seconds(&engine, 3000) == 5);
    assert(irrigation_engine_tick(&engine, 7999));
    assert(call_count == 2);

    assert(irrigation_engine_tick(&engine, 8000));
    assert(call_count == 3 && calls[2].call == CALL_ZONE_CLOSE && calls[2].relay == 3);
    assert(irrigation_engine_tick(&engine, 8999));
    assert(call_count == 3);

    assert(irrigation_engine_tick(&engine, 9000));
    assert(call_count == 4 && calls[3].call == CALL_MASTER_CLOSE);
    assert(irrigation_engine_is_idle(&engine));
}

static void test_stop_preempts_master_delay(void)
{
    irrigation_engine_t engine;
    call_count = 0;
    irrigation_engine_init(&engine);

    assert(irrigation_engine_start_zone(&engine, 2, 3, 60, 0));
    assert(irrigation_engine_stop_all(&engine));
    assert(call_count == 2);
    assert(calls[0].call == CALL_MASTER_OPEN);
    assert(calls[1].call == CALL_MASTER_CLOSE);
    assert(irrigation_engine_is_idle(&engine));
}

static void test_two_zones_run_concurrently(void)
{
    irrigation_engine_t engine;
    call_count = 0;
    irrigation_engine_init(&engine);

    assert(irrigation_engine_start_zone(&engine, 1, 1, 60, 0));
    assert(irrigation_engine_tick(&engine, 2000));
    assert(call_count == 2);
    assert(calls[0].call == CALL_MASTER_OPEN);
    assert(calls[1].call == CALL_ZONE_OPEN && calls[1].relay == 1);

    assert(irrigation_engine_start_zone(&engine, 2, 2, 45, 5000));
    assert(call_count == 3);
    assert(calls[2].call == CALL_ZONE_OPEN && calls[2].relay == 2);
    assert(engine.active_zone_id == 1);
    assert(engine.active_relay_index == 1);
    assert(irrigation_engine_remaining_seconds(&engine, 5000) == 57);

    for (size_t index = 0; index < call_count; ++index) {
        assert(calls[index].call != CALL_MASTER_CLOSE);
        assert(calls[index].call != CALL_CLOSE_ALL);
    }
}

static void test_zone_deadlines_and_stop_are_independent(void)
{
    irrigation_engine_t engine;
    call_count = 0;
    irrigation_engine_init(&engine);

    assert(irrigation_engine_start_zone(&engine, 1, 1, 5, 0));
    assert(irrigation_engine_tick(&engine, 2000));
    assert(irrigation_engine_start_zone(&engine, 2, 2, 20, 3000));
    assert(irrigation_engine_tick(&engine, 7000));
    assert(call_count == 4);
    assert(calls[3].call == CALL_ZONE_CLOSE && calls[3].relay == 1);
    assert(engine.active_zone_id == 2);
    assert(irrigation_engine_is_running(&engine));

    assert(irrigation_engine_stop_zone(&engine, 2));
    assert(call_count == 6);
    assert(calls[4].call == CALL_ZONE_CLOSE && calls[4].relay == 2);
    assert(calls[5].call == CALL_MASTER_CLOSE);
    assert(irrigation_engine_is_idle(&engine));
}

static void test_same_zone_start_refreshes_runtime(void)
{
    irrigation_engine_t engine;
    call_count = 0;
    irrigation_engine_init(&engine);

    assert(irrigation_engine_start_zone(&engine, 1, 1, 30, 0));
    assert(irrigation_engine_tick(&engine, 2000));
    assert(irrigation_engine_start_zone(&engine, 1, 1, 90, 10000));
    assert(call_count == 2);
    assert(calls[0].call == CALL_MASTER_OPEN);
    assert(calls[1].call == CALL_ZONE_OPEN && calls[1].relay == 1);
    assert(irrigation_engine_remaining_seconds(&engine, 10000) == 90);
}

static void test_same_zone_refresh_cancels_master_close_delay(void)
{
    irrigation_engine_t engine;
    call_count = 0;
    irrigation_engine_init(&engine);

    assert(irrigation_engine_start_zone(&engine, 1, 1, 5, 0));
    assert(irrigation_engine_tick(&engine, 2000));
    assert(irrigation_engine_tick(&engine, 7000));
    assert(call_count == 3);
    assert(calls[2].call == CALL_ZONE_CLOSE && calls[2].relay == 1);

    assert(irrigation_engine_start_zone(&engine, 1, 1, 30, 7500));
    assert(call_count == 4);
    assert(calls[3].call == CALL_ZONE_OPEN && calls[3].relay == 1);
    assert(irrigation_engine_remaining_seconds(&engine, 7500) == 30);

    for (size_t index = 0; index < call_count; ++index) {
        assert(calls[index].call != CALL_MASTER_CLOSE);
        assert(calls[index].call != CALL_CLOSE_ALL);
    }
}

int main(void)
{
    test_timed_relay_sequence();
    test_stop_preempts_master_delay();
    test_two_zones_run_concurrently();
    test_zone_deadlines_and_stop_are_independent();
    test_same_zone_start_refreshes_runtime();
    test_same_zone_refresh_cancels_master_close_delay();
    return 0;
}