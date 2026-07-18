#include "watersensor_protocol.h"

#include <string.h>

#define HEADER_SIZE 32u
#define FLOW_RECORD_SIZE 28u
#define TEMPERATURE_RECORD_SIZE 16u
#define CRC_OFFSET (WATERSENSOR_FRAME_SIZE - 4u)

static uint16_t read_u16_le(const uint8_t *data)
{
    return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t read_u32_le(const uint8_t *data)
{
    return (uint32_t)data[0] |
           ((uint32_t)data[1] << 8) |
           ((uint32_t)data[2] << 16) |
           ((uint32_t)data[3] << 24);
}

static uint64_t read_u64_le(const uint8_t *data)
{
    return (uint64_t)read_u32_le(data) | ((uint64_t)read_u32_le(data + 4) << 32);
}

uint32_t watersensor_protocol_crc32(const uint8_t *data, size_t length)
{
    uint32_t crc = 0xFFFFFFFFu;

    if (data == NULL) {
        return 0;
    }

    for (size_t index = 0; index < length; ++index) {
        crc ^= data[index];
        for (uint8_t bit = 0; bit < 8; ++bit) {
            crc = (crc >> 1) ^ (0xEDB88320u & (0u - (crc & 1u)));
        }
    }
    return ~crc;
}

watersensor_decode_result_t watersensor_protocol_decode(const uint8_t *frame,
                                                        size_t frame_length,
                                                        watersensor_snapshot_t *snapshot)
{
    if (frame == NULL || snapshot == NULL) {
        return WATERSENSOR_DECODE_INVALID_ARGUMENT;
    }
    if (frame_length != WATERSENSOR_FRAME_SIZE || read_u16_le(frame + 6) != WATERSENSOR_FRAME_SIZE) {
        return WATERSENSOR_DECODE_BAD_LENGTH;
    }
    if (memcmp(frame, "ZWS1", 4) != 0) {
        return WATERSENSOR_DECODE_BAD_MAGIC;
    }
    if (frame[4] != WATERSENSOR_PROTOCOL_MAJOR) {
        return WATERSENSOR_DECODE_UNSUPPORTED_VERSION;
    }
    if (read_u32_le(frame + CRC_OFFSET) != watersensor_protocol_crc32(frame, CRC_OFFSET)) {
        return WATERSENSOR_DECODE_BAD_CRC;
    }
    if (frame[30] > WATERSENSOR_MAX_FLOW_CHANNELS ||
        frame[31] > WATERSENSOR_MAX_TEMPERATURES) {
        return WATERSENSOR_DECODE_BAD_COUNT;
    }

    memset(snapshot, 0, sizeof(*snapshot));
    snapshot->protocol_minor = frame[5];
    snapshot->sequence = read_u32_le(frame + 8);
    snapshot->source_uptime_ms = read_u32_le(frame + 12);
    snapshot->boot_id = read_u32_le(frame + 16);
    snapshot->device_id = read_u64_le(frame + 20);
    snapshot->status_flags = read_u16_le(frame + 28);
    snapshot->flow_count = frame[30];
    snapshot->temperature_count = frame[31];

    for (uint8_t index = 0; index < snapshot->flow_count; ++index) {
        const uint8_t *record = frame + HEADER_SIZE + ((size_t)index * FLOW_RECORD_SIZE);
        snapshot->flows[index].channel_id = record[0];
        snapshot->flows[index].flags = read_u16_le(record + 2);
        snapshot->flows[index].pulse_count_total = read_u64_le(record + 4);
        snapshot->flows[index].volume_total_ml = read_u64_le(record + 12);
        snapshot->flows[index].flow_ml_min = read_u32_le(record + 20);
        snapshot->flows[index].last_pulse_age_ms = read_u32_le(record + 24);
    }

    const size_t temperature_offset = HEADER_SIZE +
                                      (WATERSENSOR_MAX_FLOW_CHANNELS * FLOW_RECORD_SIZE);
    for (uint8_t index = 0; index < snapshot->temperature_count; ++index) {
        const uint8_t *record = frame + temperature_offset +
                                ((size_t)index * TEMPERATURE_RECORD_SIZE);
        snapshot->temperatures[index].sensor_id = read_u64_le(record);
        snapshot->temperatures[index].temperature_millic = (int32_t)read_u32_le(record + 8);
        snapshot->temperatures[index].measurement_age_ms = read_u16_le(record + 12);
        snapshot->temperatures[index].flags = read_u16_le(record + 14);
    }

    return WATERSENSOR_DECODE_OK;
}
