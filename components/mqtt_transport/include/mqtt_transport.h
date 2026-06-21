#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mqtt_client.h"
#include "mqtt_topic.h"

typedef void (*mqtt_transport_message_cb_t)(const char *topic,
                                            size_t topic_len,
                                            const char *payload,
                                            size_t payload_len,
                                            void *user_ctx);

typedef struct {
    const char *broker_uri;
    const char *client_id;
    const char *username;
    const char *password;
    const char **subscribe_topics;
    size_t subscribe_topic_count;
    mqtt_transport_message_cb_t on_message;
    void *user_ctx;
} mqtt_transport_config_t;

typedef struct {
    esp_mqtt_client_handle_t client;
    bool connected;
    const char **subscribe_topics;
    size_t subscribe_topic_count;
    mqtt_transport_message_cb_t on_message;
    void *user_ctx;
} mqtt_transport_t;

bool mqtt_transport_init(mqtt_transport_t *transport, const mqtt_transport_config_t *config);
bool mqtt_transport_start(mqtt_transport_t *transport);
bool mqtt_transport_publish(mqtt_transport_t *transport,
                            const char *topic,
                            const char *payload,
                            int qos,
                            bool retain);
bool mqtt_transport_is_connected(const mqtt_transport_t *transport);
