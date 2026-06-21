#pragma once

#include <stdbool.h>
#include <stdint.h>

#include "mqtt_client.h"
#include "mqtt_topic.h"

typedef struct {
    const char *broker_uri;
    const char *client_id;
    const char *username;
    const char *password;
} mqtt_transport_config_t;

typedef struct {
    esp_mqtt_client_handle_t client;
    bool connected;
} mqtt_transport_t;

bool mqtt_transport_init(mqtt_transport_t *transport, const mqtt_transport_config_t *config);
bool mqtt_transport_start(mqtt_transport_t *transport);
bool mqtt_transport_publish(mqtt_transport_t *transport,
                            const char *topic,
                            const char *payload,
                            int qos,
                            bool retain);
bool mqtt_transport_is_connected(const mqtt_transport_t *transport);
