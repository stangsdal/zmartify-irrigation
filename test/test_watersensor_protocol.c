#include <assert.h>
#include <stdint.h>
#include <string.h>

#include "watersensor_protocol.h"

static void write_u16_le(uint8_t *data, uint16_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
}

static void write_u32_le(uint8_t *data, uint32_t value)
{
    data[0] = (uint8_t)value;
    data[1] = (uint8_t)(value >> 8);
    data[2] = (uint8_t)(value >> 16);
    data[3] = (uint8_t)(value >> 24);
}

static void write_u64_le(uint8_t *data, uint64_t value)
{
    write_u32_le(data, (uint32_t)value);
    write_u32_le(data + 4, (uint32_t)(value >> 32));
}

static void make_valid_frame(uint8_t frame[WATERSENSOR_FRAME_SIZE])
{
    memset(frame, 0, WATERSENSOR_FRAME_SIZE);
    memcpy(frame, "ZWS1", 4);
    frame[4] = WATERSENSOR_PROTOCOL_MAJOR;
    frame[5] = WATERSENSOR_PROTOCOL_MINOR;
    write_u16_le(frame + 6, WATERSENSOR_FRAME_SIZE);
    write_u32_le(frame + 8, 42);
    write_u32_le(frame + 12, 120000);
    write_u32_le(frame + 16, 7);
    write_u64_le(frame + 20, UINT64_C(0x1122334455667788));
    frame[30] = 1;
    frame[31] = 1;

    frame[32] = 1;
    write_u16_le(frame + 34, WATERSENSOR_MEASUREMENT_VALID);
    write_u64_le(frame + 36, 123456);
    write_u64_le(frame + 44, 987654);
    write_u32_le(frame + 52, 14500);
    write_u32_le(frame + 56, 25);

    const size_t temperature_offset = 32 + (WATERSENSOR_MAX_FLOW_CHANNELS * 28);
    write_u64_le(frame + temperature_offset, UINT64_C(0x2801020304050607));
    write_u32_le(frame + temperature_offset + 8, 21500);
    write_u16_le(frame + temperature_offset + 12, 100);
    write_u16_le(frame + temperature_offset + 14, WATERSENSOR_MEASUREMENT_VALID);

    write_u32_le(frame + WATERSENSOR_FRAME_SIZE - 4,
                 watersensor_protocol_crc32(frame, WATERSENSOR_FRAME_SIZE - 4));
}

static void test_valid_frame(void)
{
    uint8_t frame[WATERSENSOR_FRAME_SIZE];
    watersensor_snapshot_t snapshot;
    make_valid_frame(frame);

    assert(watersensor_protocol_decode(frame, sizeof(frame), &snapshot) == WATERSENSOR_DECODE_OK);
    assert(snapshot.sequence == 42);
    assert(snapshot.boot_id == 7);
    assert(snapshot.device_id == UINT64_C(0x1122334455667788));
    assert(snapshot.flow_count == 1);
    assert(snapshot.flows[0].channel_id == 1);
    assert(snapshot.flows[0].pulse_count_total == 123456);
    assert(snapshot.flows[0].volume_total_ml == 987654);
    assert(snapshot.flows[0].flow_ml_min == 14500);
    assert(snapshot.temperatures[0].temperature_millic == 21500);
}

static void test_rejections(void)
{
    uint8_t frame[WATERSENSOR_FRAME_SIZE];
    watersensor_snapshot_t snapshot;

    make_valid_frame(frame);
    frame[40] ^= 1;
    assert(watersensor_protocol_decode(frame, sizeof(frame), &snapshot) == WATERSENSOR_DECODE_BAD_CRC);

    make_valid_frame(frame);
    frame[4]++;
    write_u32_le(frame + WATERSENSOR_FRAME_SIZE - 4,
                 watersensor_protocol_crc32(frame, WATERSENSOR_FRAME_SIZE - 4));
    assert(watersensor_protocol_decode(frame, sizeof(frame), &snapshot) ==
           WATERSENSOR_DECODE_UNSUPPORTED_VERSION);

    make_valid_frame(frame);
    assert(watersensor_protocol_decode(frame, sizeof(frame) - 1, &snapshot) ==
           WATERSENSOR_DECODE_BAD_LENGTH);
}

int main(void)
{
    test_valid_frame();
    test_rejections();
    return 0;
}
