#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define WATERSENSOR_PROTOCOL_MAJOR 1u
#define WATERSENSOR_PROTOCOL_MINOR 0u
#define WATERSENSOR_FRAME_SIZE 228u
#define WATERSENSOR_MAX_FLOW_CHANNELS 4u
#define WATERSENSOR_MAX_TEMPERATURES 5u

#define WATERSENSOR_MEASUREMENT_VALID (1u << 0)
#define WATERSENSOR_MEASUREMENT_FAULT (1u << 1)

typedef enum {
    WATERSENSOR_DECODE_OK = 0,
    WATERSENSOR_DECODE_INVALID_ARGUMENT,
    WATERSENSOR_DECODE_BAD_MAGIC,
    WATERSENSOR_DECODE_BAD_LENGTH,
    WATERSENSOR_DECODE_UNSUPPORTED_VERSION,
    WATERSENSOR_DECODE_BAD_CRC,
    WATERSENSOR_DECODE_BAD_COUNT,
} watersensor_decode_result_t;

typedef struct {
    uint8_t channel_id;
    uint16_t flags;
    uint64_t pulse_count_total;
    uint64_t volume_total_ml;
    uint32_t flow_ml_min;
    uint32_t last_pulse_age_ms;
} watersensor_flow_record_t;

typedef struct {
    uint64_t sensor_id;
    int32_t temperature_millic;
    uint16_t measurement_age_ms;
    uint16_t flags;
} watersensor_temperature_record_t;

typedef struct {
    uint8_t protocol_minor;
    uint32_t sequence;
    uint32_t source_uptime_ms;
    uint32_t boot_id;
    uint64_t device_id;
    uint16_t status_flags;
    uint8_t flow_count;
    uint8_t temperature_count;
    watersensor_flow_record_t flows[WATERSENSOR_MAX_FLOW_CHANNELS];
    watersensor_temperature_record_t temperatures[WATERSENSOR_MAX_TEMPERATURES];
} watersensor_snapshot_t;

uint32_t watersensor_protocol_crc32(const uint8_t *data, size_t length);
watersensor_decode_result_t watersensor_protocol_decode(const uint8_t *frame,
                                                        size_t frame_length,
                                                        watersensor_snapshot_t *snapshot);
