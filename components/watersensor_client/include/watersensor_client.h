#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "watersensor_protocol.h"

#define WATERSENSOR_DEFAULT_I2C_ADDRESS 0x31u
#define WATERSENSOR_DEFAULT_ACTIVE_POLL_MS 500u
#define WATERSENSOR_DEFAULT_OFFLINE_POLL_MS 5000u
#define WATERSENSOR_DEFAULT_MAX_AGE_MS 1500u
#define WATERSENSOR_DEFAULT_FLOW_CHANNEL 1u

typedef enum {
    WATERSENSOR_LINK_UNKNOWN = 0,
    WATERSENSOR_LINK_OFFLINE,
    WATERSENSOR_LINK_ONLINE,
    WATERSENSOR_LINK_PROTOCOL_ERROR,
    WATERSENSOR_LINK_STALE,
} watersensor_link_state_t;

typedef struct {
    uint8_t i2c_address;
    uint32_t online_poll_interval_ms;
    uint32_t offline_poll_interval_ms;
    uint32_t maximum_snapshot_age_ms;
    uint64_t expected_device_id;
    uint8_t flow_channel_id;
} watersensor_client_config_t;

typedef struct {
    watersensor_client_config_t config;
    watersensor_snapshot_t snapshot;
    watersensor_link_state_t state;
    int64_t received_at_us;
    uint32_t last_sequence;
    uint32_t last_boot_id;
    bool has_snapshot;
    bool initialized;
} watersensor_client_t;

bool watersensor_client_init(watersensor_client_t *client,
                             const watersensor_client_config_t *config);
bool watersensor_client_poll(watersensor_client_t *client);
bool watersensor_client_get_snapshot(watersensor_client_t *client,
                                     watersensor_snapshot_t *snapshot,
                                     uint32_t *measurement_age_ms);
watersensor_link_state_t watersensor_client_get_state(watersensor_client_t *client);
uint32_t watersensor_client_next_poll_ms(const watersensor_client_t *client);
