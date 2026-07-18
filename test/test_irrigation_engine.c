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

static relay_call_t calls[8];
static size_t call_count;

relay_result_t relay_master_open(void)
{
    calls[call_count++] = CALL_MASTER_OPEN;
    return RELAY_OK;
}

relay_result_t relay_master_close(void)
{
    calls[call_count++] = CALL_MASTER_CLOSE;
    return RELAY_OK;
}

relay_result_t relay_zone_open(uint8_t relay)
{
    assert(relay == 3);
    calls[call_count++] = CALL_ZONE_OPEN;
    return RELAY_OK;
}

relay_result_t relay_zone_close(uint8_t relay)
{
    assert(relay == 3);
    calls[call_count++] = CALL_ZONE_CLOSE;
    return RELAY_OK;
}

relay_result_t relay_close_all(void)
{
    calls[call_count++] = CALL_CLOSE_ALL;
    return RELAY_OK;
}

static void test_timed_relay_sequence(void)
{
    irrigation_engine_t engine;
    call_count = 0;
    irrigation_engine_init(&engine);

    assert(irrigation_engine_start_zone(&engine, 2, 3, 5, 1000));
    assert(call_count == 1 && calls[0] == CALL_MASTER_OPEN);
    assert(irrigation_engine_tick(&engine, 2999));
    assert(call_count == 1);

    assert(irrigation_engine_tick(&engine, 3000));
    assert(call_count == 2 && calls[1] == CALL_ZONE_OPEN);
    assert(irrigation_engine_remaining_seconds(&engine, 3000) == 5);
    assert(irrigation_engine_tick(&engine, 7999));
    assert(call_count == 2);

    assert(irrigation_engine_tick(&engine, 8000));
    assert(call_count == 3 && calls[2] == CALL_ZONE_CLOSE);
    assert(irrigation_engine_tick(&engine, 8999));
    assert(call_count == 3);

    assert(irrigation_engine_tick(&engine, 9000));
    assert(call_count == 4 && calls[3] == CALL_MASTER_CLOSE);
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
    assert(calls[0] == CALL_MASTER_OPEN);
    assert(calls[1] == CALL_MASTER_CLOSE);
    assert(irrigation_engine_is_idle(&engine));
}

int main(void)
{
    test_timed_relay_sequence();
    test_stop_preempts_master_delay();
    return 0;
}