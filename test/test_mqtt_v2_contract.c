#include <assert.h>
#include <string.h>

#include "zic_v2.h"

#define NOW_EPOCH 1784487600u

static zic_v2_command_t valid_start(const char *command_id)
{
    zic_v2_command_t command = {
              .authorized = true,
        .source_epoch_s = NOW_EPOCH,
        .action = ZIC_V2_ACTION_ZONE_START,
        .zone_id = 3,
        .runtime_seconds = 600,
    };
    strncpy(command.command_id, command_id, sizeof(command.command_id) - 1u);
    return command;
}

static void test_timestamp_parser(void)
{
    uint32_t epoch = 0;
    assert(zic_v2_parse_utc_timestamp("2026-07-19T19:00:00Z", &epoch));
    assert(epoch == 1784487600u);
    assert(zic_v2_parse_utc_timestamp("2024-02-29T00:00:00Z", &epoch));
    assert(!zic_v2_parse_utc_timestamp("2025-02-29T00:00:00Z", &epoch));
    assert(!zic_v2_parse_utc_timestamp("2026-07-19 19:00:00Z", &epoch));
    assert(!zic_v2_parse_utc_timestamp("2026-07-19T19:00:00+00:00", &epoch));
}

static void test_accept_and_duplicate(void)
{
    zic_v2_command_tracker_t tracker = {0};
    zic_v2_command_reason_t reason;
    zic_v2_command_t command = valid_start("cmd-001");

    assert(zic_v2_validate_command(&command, NOW_EPOCH, &tracker, &reason) ==
           ZIC_V2_COMMAND_ACCEPTED);
    assert(reason == ZIC_V2_REASON_NONE);
    assert(zic_v2_validate_command(&command, NOW_EPOCH, &tracker, &reason) ==
           ZIC_V2_COMMAND_DUPLICATE);
    assert(reason == ZIC_V2_REASON_NONE);
}

static void test_schema_and_range_rejections(void)
{
    zic_v2_command_tracker_t tracker = {0};
    zic_v2_command_reason_t reason;
    zic_v2_command_t command = valid_start("bad id");
    assert(zic_v2_validate_command(&command, NOW_EPOCH, &tracker, &reason) ==
           ZIC_V2_COMMAND_REJECTED);
    assert(reason == ZIC_V2_REASON_INVALID_ID);

    command = valid_start("unauthorized");
    command.authorized = false;
    assert(zic_v2_validate_command(&command, NOW_EPOCH, &tracker, &reason) ==
           ZIC_V2_COMMAND_REJECTED);
    assert(reason == ZIC_V2_REASON_UNAUTHORIZED);

    command = valid_start("stale");
    command.source_epoch_s = NOW_EPOCH - 301u;
    assert(zic_v2_validate_command(&command, NOW_EPOCH, &tracker, &reason) ==
           ZIC_V2_COMMAND_REJECTED);
    assert(reason == ZIC_V2_REASON_STALE);

    command = valid_start("future");
    command.source_epoch_s = NOW_EPOCH + 31u;
    assert(zic_v2_validate_command(&command, NOW_EPOCH, &tracker, &reason) ==
           ZIC_V2_COMMAND_REJECTED);
    assert(reason == ZIC_V2_REASON_FUTURE);

    command = valid_start("zone");
       command.zone_id = 257;
    assert(zic_v2_validate_command(&command, NOW_EPOCH, &tracker, &reason) ==
           ZIC_V2_COMMAND_REJECTED);
    assert(reason == ZIC_V2_REASON_INVALID_ZONE);

    command = valid_start("runtime");
    command.runtime_seconds = 7201;
    assert(zic_v2_validate_command(&command, NOW_EPOCH, &tracker, &reason) ==
           ZIC_V2_COMMAND_REJECTED);
    assert(reason == ZIC_V2_REASON_INVALID_RUNTIME);

    command = valid_start("rain");
    command.action = ZIC_V2_ACTION_RAIN_DELAY;
    command.zone_id = 0;
    command.runtime_seconds = 0;
       command.rain_delay_hours = 65536;
    assert(zic_v2_validate_command(&command, NOW_EPOCH, &tracker, &reason) ==
           ZIC_V2_COMMAND_REJECTED);
    assert(reason == ZIC_V2_REASON_INVALID_RAIN_DELAY);
}

static void test_result_names(void)
{
    assert(strcmp(zic_v2_command_decision_name(ZIC_V2_COMMAND_ACCEPTED), "accepted") == 0);
    assert(strcmp(zic_v2_command_decision_name(ZIC_V2_COMMAND_REJECTED), "rejected") == 0);
    assert(strcmp(zic_v2_command_decision_name(ZIC_V2_COMMAND_DUPLICATE), "duplicate") == 0);
    assert(strcmp(zic_v2_command_reason_name(ZIC_V2_REASON_STALE), "stale_command") == 0);
}

int main(void)
{
    test_timestamp_parser();
    test_accept_and_duplicate();
    test_schema_and_range_rejections();
    test_result_names();
    return 0;
}