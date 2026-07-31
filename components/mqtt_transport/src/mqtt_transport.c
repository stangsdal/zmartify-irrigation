#include "mqtt_transport.h"

#include <errno.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

#include "esp_crt_bundle.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "mqtt_client.h"

static const char *TAG = "mqtt_transport";

static void mqtt_transport_reset_rx_buffer(mqtt_transport_t *transport)
{
    if (transport == NULL) {
        return;
    }

    free(transport->rx_topic);
    free(transport->rx_payload);
    transport->rx_topic = NULL;
    transport->rx_topic_len = 0u;
    transport->rx_payload = NULL;
    transport->rx_payload_len = 0u;
    transport->rx_payload_capacity = 0u;
}

static void mqtt_transport_set_error(mqtt_transport_t *transport, const char *message)
{
    if (transport == NULL) {
        return;
    }
    if (message == NULL || message[0] == '\0') {
        transport->last_error[0] = '\0';
        return;
    }
    (void)snprintf(transport->last_error, sizeof(transport->last_error), "%s", message);
}

static const char *mqtt_transport_refused_reason(int code)
{
    switch (code) {
    case 1:
        return "unacceptable protocol version";
    case 2:
        return "identifier rejected";
    case 3:
        return "server unavailable";
    case 4:
        return "bad username or password";
    case 5:
        return "not authorized";
    default:
        return "connection refused";
    }
}

static void mqtt_transport_capture_error(mqtt_transport_t *transport, esp_mqtt_event_handle_t event)
{
    if (transport == NULL) {
        return;
    }
    ++transport->error_count;

    const esp_mqtt_error_codes_t *err = event != NULL ? event->error_handle : NULL;
    if (err == NULL) {
        mqtt_transport_set_error(transport, "mqtt error: missing detail");
        return;
    }

    char message[128];
    switch (err->error_type) {
    case MQTT_ERROR_TYPE_CONNECTION_REFUSED:
        (void)snprintf(message, sizeof(message), "%s (rc=%d)",
                       mqtt_transport_refused_reason(err->connect_return_code),
                       (int)err->connect_return_code);
        mqtt_transport_set_error(transport, message);
        break;
    case MQTT_ERROR_TYPE_SUBSCRIBE_FAILED:
        mqtt_transport_set_error(transport, "subscribe failed: broker rejected topic/qos");
        break;
    case MQTT_ERROR_TYPE_TCP_TRANSPORT:
        if (err->esp_transport_sock_errno == ETIMEDOUT) {
            mqtt_transport_set_error(transport, "transport timeout (tcp)");
        } else if (err->esp_transport_sock_errno == ECONNREFUSED) {
            mqtt_transport_set_error(transport, "transport connection refused");
        } else if (err->esp_transport_sock_errno == ENETUNREACH ||
                   err->esp_transport_sock_errno == EHOSTUNREACH) {
            mqtt_transport_set_error(transport, "transport unreachable");
        } else {
            (void)snprintf(message, sizeof(message),
                           "transport/tls error (esp=0x%x,tls=0x%x,errno=%d:%s)",
                           (unsigned int)err->esp_tls_last_esp_err,
                           (unsigned int)err->esp_tls_stack_err,
                           err->esp_transport_sock_errno,
                           err->esp_transport_sock_errno != 0 ? strerror(err->esp_transport_sock_errno) : "none");
            mqtt_transport_set_error(transport, message);
        }
        break;
    case MQTT_ERROR_TYPE_NONE:
        mqtt_transport_set_error(transport, "mqtt error: unknown source");
        break;
    default:
        (void)snprintf(message, sizeof(message), "mqtt error type=%d", (int)err->error_type);
        mqtt_transport_set_error(transport, message);
        break;
    }
}

static void mqtt_event_handler(void *handler_args,
                               esp_event_base_t base,
                               int32_t event_id,
                               void *event_data)
{
    (void)base;

    mqtt_transport_t *transport = (mqtt_transport_t *)handler_args;
    if (transport == NULL) {
        return;
    }

    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        transport->connected = true;
        transport->disconnected_since_us = 0;
        ++transport->connect_count;
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
        if (transport->disconnected_since_us == 0) {
            transport->disconnected_since_us = esp_timer_get_time();
        }
        ++transport->disconnect_count;
        mqtt_transport_reset_rx_buffer(transport);
        if (transport->last_error[0] == '\0') {
            mqtt_transport_set_error(transport, "disconnected");
        }
        ESP_LOGW(TAG, "MQTT disconnected");
        break;
    case MQTT_EVENT_ERROR:
        mqtt_transport_capture_error(transport, (esp_mqtt_event_handle_t)event_data);
        ESP_LOGW(TAG, "MQTT error: %s", transport->last_error);
        break;
    case MQTT_EVENT_DATA: {
        esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
        if (transport->on_message != NULL && event != NULL) {
            size_t topic_len = (size_t)event->topic_len;
            size_t data_len = (size_t)event->data_len;
            size_t total_len = (size_t)event->total_data_len;
            size_t offset = (size_t)event->current_data_offset;

            if (total_len == 0u || (offset == 0u && total_len == data_len)) {
                transport->on_message(event->topic,
                                      topic_len,
                                      event->data,
                                      data_len,
                                      transport->user_ctx);
                break;
            }

            if (offset == 0u) {
                mqtt_transport_reset_rx_buffer(transport);
                transport->rx_topic = calloc(topic_len + 1u, sizeof(char));
                transport->rx_payload = calloc(total_len + 1u, sizeof(char));
                if (transport->rx_topic == NULL || transport->rx_payload == NULL) {
                    ESP_LOGW(TAG, "MQTT fragment allocation failed (%u bytes)", (unsigned)total_len);
                    mqtt_transport_reset_rx_buffer(transport);
                    break;
                }
                memcpy(transport->rx_topic, event->topic, topic_len);
                transport->rx_topic_len = topic_len;
                transport->rx_payload_capacity = total_len;
            }

            if (transport->rx_topic == NULL || transport->rx_payload == NULL ||
                offset + data_len > transport->rx_payload_capacity) {
                ESP_LOGW(TAG, "MQTT fragment assembly failed (offset=%u len=%u total=%u)",
                         (unsigned)offset, (unsigned)data_len, (unsigned)total_len);
                mqtt_transport_reset_rx_buffer(transport);
                break;
            }

            memcpy(transport->rx_payload + offset, event->data, data_len);
            if (offset + data_len > transport->rx_payload_len) {
                transport->rx_payload_len = offset + data_len;
            }

            if (transport->rx_payload_len >= transport->rx_payload_capacity) {
                transport->on_message(transport->rx_topic,
                                      transport->rx_topic_len,
                                      transport->rx_payload,
                                      transport->rx_payload_len,
                                      transport->user_ctx);
                mqtt_transport_reset_rx_buffer(transport);
            }
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
    transport->connect_count = 0;
    transport->disconnect_count = 0;
    transport->error_count = 0;
    transport->recovery_count = 0;
    transport->disconnected_since_us = 0;
    transport->last_error[0] = '\0';
    transport->subscribe_topics = config->subscribe_topics;
    transport->subscribe_topic_count = config->subscribe_topic_count;
    transport->on_message = config->on_message;
    transport->on_connected = config->on_connected;
    transport->user_ctx = config->user_ctx;
    transport->rx_topic = NULL;
    transport->rx_topic_len = 0u;
    transport->rx_payload = NULL;
    transport->rx_payload_len = 0u;
    transport->rx_payload_capacity = 0u;
    if (transport->client == NULL) {
        mqtt_transport_set_error(transport, "client init failed");
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

    if (esp_mqtt_client_start(transport->client) != ESP_OK) {
        mqtt_transport_set_error(transport, "client start failed");
        return false;
    }
    return true;
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

bool mqtt_transport_recover_if_stale(mqtt_transport_t *transport, uint32_t max_disconnected_ms)
{
    if (transport == NULL || transport->client == NULL || transport->connected ||
        transport->disconnected_since_us == 0 || max_disconnected_ms == 0) {
        return false;
    }

    int64_t now_us = esp_timer_get_time();
    int64_t max_disconnected_us = (int64_t)max_disconnected_ms * 1000LL;
    if (now_us - transport->disconnected_since_us < max_disconnected_us) {
        return false;
    }

    ESP_LOGW(TAG, "MQTT disconnected for %u ms; restarting client", (unsigned)max_disconnected_ms);
    mqtt_transport_reset_rx_buffer(transport);
    esp_err_t stop_err = esp_mqtt_client_stop(transport->client);
    esp_err_t start_err = esp_mqtt_client_start(transport->client);
    transport->disconnected_since_us = now_us;
    ++transport->recovery_count;
    if (stop_err != ESP_OK || start_err != ESP_OK) {
        char message[128];
        (void)snprintf(message, sizeof(message), "mqtt recovery failed (stop=%s,start=%s)",
                       esp_err_to_name(stop_err), esp_err_to_name(start_err));
        mqtt_transport_set_error(transport, message);
        return false;
    }
    return true;
}

void mqtt_transport_get_status(const mqtt_transport_t *transport, mqtt_transport_status_t *out)
{
    if (out == NULL) {
        return;
    }
    memset(out, 0, sizeof(*out));
    if (transport == NULL) {
        return;
    }
    out->connected = transport->connected;
    out->connect_count = transport->connect_count;
    out->disconnect_count = transport->disconnect_count;
    out->error_count = transport->error_count;
    out->recovery_count = transport->recovery_count;
    (void)snprintf(out->last_error, sizeof(out->last_error), "%s", transport->last_error);
}
