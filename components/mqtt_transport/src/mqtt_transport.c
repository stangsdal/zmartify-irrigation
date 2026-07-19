#include "mqtt_transport.h"

#include <stddef.h>

#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "mqtt_client.h"
#include "mqtt5_client.h"

static const char *TAG = "mqtt_transport";

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    (void)base;
    (void)event_data;

    mqtt_transport_t *transport = (mqtt_transport_t *)handler_args;
    if (transport == NULL) {
        return;
    }

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        transport->connected = true;
        ESP_LOGI(TAG, "MQTT connected");
        for (size_t i = 0; i < transport->subscribe_topic_count; ++i) {
            if (transport->subscribe_topics[i] != NULL) {
                esp_mqtt_client_subscribe(transport->client, transport->subscribe_topics[i],
                                          MQTT_TRANSPORT_QOS_COMMAND);
            }
        }
        if (transport->on_connected != NULL) {
            transport->on_connected(transport->user_ctx);
        }
        break;
    case MQTT_EVENT_DISCONNECTED:
        transport->connected = false;
        ESP_LOGW(TAG, "MQTT disconnected");
        break;
    case MQTT_EVENT_DATA: {
        esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
        if (transport->on_message != NULL && event != NULL) {
            transport->on_message(event->topic,
                                  (size_t)event->topic_len,
                                  event->data,
                                  (size_t)event->data_len,
                                  transport->user_ctx);
        }
        break;
    }
    default:
        break;
    }
}

bool mqtt_transport_init(mqtt_transport_t *transport, const mqtt_transport_config_t *config)
{
    if (transport == NULL || config == NULL || config->broker_uri == NULL) {
        return false;
    }

    const esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = config->broker_uri,
        .broker.verification.crt_bundle_attach = config->use_crt_bundle ? esp_crt_bundle_attach : NULL,
        .credentials.client_id = config->client_id,
        .credentials.username = config->username,
        .credentials.authentication.password = config->password,
        .session.protocol_ver = MQTT_PROTOCOL_V_5,
        .session.last_will.topic = config->last_will_topic,
        .session.last_will.msg = config->last_will_message,
        .session.last_will.qos = MQTT_TRANSPORT_QOS_STATE,
        .session.last_will.retain = true,
        .session.disable_clean_session = true,
        .network.disable_auto_reconnect = false,
        .network.reconnect_timeout_ms = 5000,
    };

    transport->client = esp_mqtt_client_init(&mqtt_cfg);
    transport->connected = false;
    transport->subscribe_topics = config->subscribe_topics;
    transport->subscribe_topic_count = config->subscribe_topic_count;
    transport->on_message = config->on_message;
    transport->on_connected = config->on_connected;
    transport->user_ctx = config->user_ctx;
    if (transport->client == NULL) {
        return false;
    }

    esp_mqtt5_connection_property_config_t connection_properties = {
        .session_expiry_interval = 3600,
        .maximum_packet_size = 2048,
        .receive_maximum = 16,
        .request_problem_info = true,
    };
    if (esp_mqtt5_client_set_connect_property(transport->client,
                                               &connection_properties) != ESP_OK) {
        esp_mqtt_client_destroy(transport->client);
        transport->client = NULL;
        return false;
    }

    esp_mqtt_client_register_event(transport->client, ESP_EVENT_ANY_ID, mqtt_event_handler, transport);
    return true;
}

bool mqtt_transport_start(mqtt_transport_t *transport)
{
    if (transport == NULL || transport->client == NULL) {
        return false;
    }

    return esp_mqtt_client_start(transport->client) == ESP_OK;
}

bool mqtt_transport_publish(mqtt_transport_t *transport,
                            const char *topic,
                            const char *payload,
                            int qos,
                            bool retain)
{
    if (transport == NULL || transport->client == NULL || topic == NULL || payload == NULL) {
        return false;
    }

    int msg_id = esp_mqtt_client_publish(transport->client, topic, payload, 0, qos, retain);
    return msg_id >= 0;
}

bool mqtt_transport_is_connected(const mqtt_transport_t *transport)
{
    if (transport == NULL) {
        return false;
    }

    return transport->connected;
}
