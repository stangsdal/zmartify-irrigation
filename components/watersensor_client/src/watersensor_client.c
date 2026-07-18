#include "watersensor_client.h"

#include <string.h>

#include "esp_log.h"
#include "esp_timer.h"
#include "event_bus.h"
#include "hal.h"

static const char *TAG = "watersensor";

typedef struct {
    uint32_t sequence;
    uint32_t boot_id;
    uint64_t device_id;
    uint8_t flow_count;
    uint8_t temperature_count;
} watersensor_snapshot_event_t;

static void set_state(watersensor_client_t *client, watersensor_link_state_t state)
{
    if (client->state == state) {
        return;
    }

    client->state = state;
    uint32_t event_id = EVENT_WATERSENSOR_OFFLINE;
    uint8_t priority = EVENT_PRIORITY_HIGH;

    if (state == WATERSENSOR_LINK_ONLINE) {
        event_id = EVENT_WATERSENSOR_ONLINE;
        priority = EVENT_PRIORITY_NORMAL;
        ESP_LOGI(TAG, "Water Sensor online at I2C 0x%02X", client->config.i2c_address);
    } else if (state == WATERSENSOR_LINK_PROTOCOL_ERROR) {
        event_id = EVENT_WATERSENSOR_PROTOCOL_ERROR;
        ESP_LOGW(TAG, "Water Sensor protocol error");
    } else if (state == WATERSENSOR_LINK_STALE) {
        event_id = EVENT_SENSOR_STALE;
        ESP_LOGW(TAG, "Water Sensor snapshot stale");
    } else {
        ESP_LOGI(TAG, "Water Sensor offline; optional source will be retried");
    }

    (void)event_bus_publish(event_id, 0, priority, 0, NULL, 0);
}

bool watersensor_client_init(watersensor_client_t *client,
                             const watersensor_client_config_t *config)
{
    if (client == NULL) {
        return false;
    }

    memset(client, 0, sizeof(*client));
    client->config.i2c_address = WATERSENSOR_DEFAULT_I2C_ADDRESS;
    client->config.online_poll_interval_ms = WATERSENSOR_DEFAULT_ACTIVE_POLL_MS;
    client->config.offline_poll_interval_ms = WATERSENSOR_DEFAULT_OFFLINE_POLL_MS;
    client->config.maximum_snapshot_age_ms = WATERSENSOR_DEFAULT_MAX_AGE_MS;
    client->config.flow_channel_id = WATERSENSOR_DEFAULT_FLOW_CHANNEL;

    if (config != NULL) {
        client->config = *config;
    }
    if (client->config.i2c_address == 0 ||
        client->config.online_poll_interval_ms == 0 ||
        client->config.offline_poll_interval_ms == 0 ||
        client->config.maximum_snapshot_age_ms == 0 ||
        client->config.flow_channel_id == 0) {
        return false;
    }

    client->state = WATERSENSOR_LINK_UNKNOWN;
    client->initialized = true;
    ESP_LOGI(TAG, "Water Sensor client ready at I2C 0x%02X (device may be absent)",
             client->config.i2c_address);
    return true;
}

bool watersensor_client_poll(watersensor_client_t *client)
{
    if (client == NULL || !client->initialized) {
        return false;
    }

    if (!hal_i2c_probe(client->config.i2c_address)) {
        set_state(client, WATERSENSOR_LINK_OFFLINE);
        return false;
    }

    uint8_t frame[WATERSENSOR_FRAME_SIZE];
    if (hal_i2c_read(client->config.i2c_address, frame, sizeof(frame)) != HAL_OK) {
        set_state(client, WATERSENSOR_LINK_OFFLINE);
        return false;
    }

    watersensor_snapshot_t snapshot;
    watersensor_decode_result_t result =
        watersensor_protocol_decode(frame, sizeof(frame), &snapshot);
    if (result != WATERSENSOR_DECODE_OK) {
        ESP_LOGW(TAG, "Rejected Water Sensor frame: %d", result);
        set_state(client, WATERSENSOR_LINK_PROTOCOL_ERROR);
        return false;
    }
    if (client->config.expected_device_id != 0 &&
        snapshot.device_id != client->config.expected_device_id) {
        ESP_LOGW(TAG, "Rejected unexpected Water Sensor identity");
        set_state(client, WATERSENSOR_LINK_PROTOCOL_ERROR);
        return false;
    }

    const bool new_source_boot = !client->has_snapshot || snapshot.boot_id != client->last_boot_id;
    const bool new_sequence = new_source_boot || snapshot.sequence != client->last_sequence;
    if (new_sequence) {
        client->snapshot = snapshot;
        client->last_sequence = snapshot.sequence;
        client->last_boot_id = snapshot.boot_id;
        client->received_at_us = esp_timer_get_time();
        client->has_snapshot = true;
        const watersensor_snapshot_event_t event = {
            .sequence = snapshot.sequence,
            .boot_id = snapshot.boot_id,
            .device_id = snapshot.device_id,
            .flow_count = snapshot.flow_count,
            .temperature_count = snapshot.temperature_count,
        };
        (void)event_bus_publish(EVENT_WATERSENSOR_SNAPSHOT, 0, EVENT_PRIORITY_NORMAL,
                                0, &event, sizeof(event));
        set_state(client, WATERSENSOR_LINK_ONLINE);
        return true;
    }

    return watersensor_client_get_state(client) == WATERSENSOR_LINK_ONLINE;
}

bool watersensor_client_get_snapshot(watersensor_client_t *client,
                                     watersensor_snapshot_t *snapshot,
                                     uint32_t *measurement_age_ms)
{
    if (client == NULL || snapshot == NULL || !client->has_snapshot) {
        return false;
    }

    int64_t age_us = esp_timer_get_time() - client->received_at_us;
    uint32_t age_ms = age_us > 0 ? (uint32_t)(age_us / 1000) : 0;
    if (measurement_age_ms != NULL) {
        *measurement_age_ms = age_ms;
    }
    if (age_ms > client->config.maximum_snapshot_age_ms) {
        set_state(client, WATERSENSOR_LINK_STALE);
        return false;
    }

    *snapshot = client->snapshot;
    return true;
}

watersensor_link_state_t watersensor_client_get_state(watersensor_client_t *client)
{
    if (client == NULL || !client->initialized || client->state == WATERSENSOR_LINK_UNKNOWN) {
        return WATERSENSOR_LINK_OFFLINE;
    }

    if (client->has_snapshot) {
        int64_t age_us = esp_timer_get_time() - client->received_at_us;
        if (age_us > (int64_t)client->config.maximum_snapshot_age_ms * 1000) {
            set_state(client, WATERSENSOR_LINK_STALE);
        }
    }
    return client->state;
}

uint32_t watersensor_client_next_poll_ms(const watersensor_client_t *client)
{
    if (client == NULL || !client->initialized) {
        return WATERSENSOR_DEFAULT_OFFLINE_POLL_MS;
    }
    return client->state == WATERSENSOR_LINK_ONLINE
               ? client->config.online_poll_interval_ms
               : client->config.offline_poll_interval_ms;
}
