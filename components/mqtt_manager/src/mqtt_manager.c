
/*
 * mqtt_manager.c - MQTT Manager v5.0
 */
#include "mqtt_manager.h"
#include "config_manager.h"
#include "event_bus.h"
#include "irrigation_engine.h"
#include "weather_manager.h"
#include "flow_manager.h"
#include "pressure_manager.h"
#include "alarm_manager.h"
#include "storage_manager.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>

static const char *TAG = "mqtt_mgr";

#define ROOT       "zmartify/irrigation/" "controller_01"
#define TOPIC_STATE      ROOT "/state"
#define TOPIC_TELEMETRY  ROOT "/telemetry"
#define TOPIC_ALARM      ROOT "/alarm"
#define TOPIC_CMD_ALL    ROOT "/command/#"
#define TELEMETRY_PERIOD_MS 30000u
#define WIFI_MAX_RETRY 10

static mqtt_mgr_state_t          s_state       = MQTT_MGR_WIFI_DISCONNECTED;
static esp_mqtt_client_handle_t  s_client       = NULL;
static bool                      s_initialized  = false;
static uint8_t                   s_wifi_retries = 0;

static bool safe_publish(const char *topic, const char *payload, int qos, int retain)
{
    if (!s_client || s_state != MQTT_MGR_MQTT_CONNECTED) return false;
    return esp_mqtt_client_publish(s_client, topic, payload, 0, qos, retain) >= 0;
}

static void publish_state(void)
{
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"engine\":%d,\"zone\":%u,\"alarms\":%u,"
             "\"flow_state\":%d,\"pres_state\":%d}",
             (int)irrigation_get_state(),
             (unsigned)irrigation_active_zone(),
             (unsigned)alarm_active_count(),
             (int)flow_manager_get_state(),
             (int)pressure_manager_get_state());
    safe_publish(TOPIC_STATE, buf, 1, 1);
}

static void publish_telemetry(void)
{
    char buf[256];
    snprintf(buf, sizeof(buf),
             "{\"flow_lpm\":%.2f,\"pressure_bar\":%.2f,\"logs\":%u}",
             flow_manager_get_rate_lpm(),
             pressure_manager_get_bar(),
             (unsigned)storage_manager_count());
    safe_publish(TOPIC_TELEMETRY, buf, 0, 0);
}

static void on_bus_event(const zic_event_t *ev, void *ctx)
{
    (void)ctx;
    if (s_state != MQTT_MGR_MQTT_CONNECTED) return;
    switch (ev->event_id)
    {
        case EVENT_ZONE_STARTED: case EVENT_ZONE_STOPPED:
        case EVENT_IRRIGATION_STARTED: case EVENT_IRRIGATION_COMPLETED:
        case EVENT_IRRIGATION_FAULT:
            publish_state();
            break;
        case EVENT_ALARM_GENERATED: {
            char buf[96];
            snprintf(buf, sizeof(buf), "{\"code\":%u,\"sev\":%u,\"zone\":%u}",
                     ev->payload_size > 0 ? ev->payload[0] : 0u,
                     ev->payload_size > 1 ? ev->payload[1] : 0u,
                     ev->payload_size > 2 ? ev->payload[2] : 0u);
            safe_publish(TOPIC_ALARM, buf, 1, 0);
            break;
        }
        default: break;
    }
}

static void handle_command(const char *cmd, const char *payload)
{
    ESP_LOGI(TAG, "CMD: %s  payload: %s", cmd, payload);
    if (strcmp(cmd, "stop_all") == 0)
    {
        irrigation_stop_all();
    }
    else if (strcmp(cmd, "stop_zone") == 0)
    {
        unsigned z = 0;
        sscanf(payload, "{\"zone_id\":%u", &z);
        if (z >= 1 && z <= 15) irrigation_stop_zone((uint8_t)z);
    }
    else if (strcmp(cmd, "start_zone") == 0)
    {
        unsigned z = 0, rt = 0;
        sscanf(payload, "{\"zone_id\":%u,\"runtime_s\":%u", &z, &rt);
        if (z >= 1 && z <= 15)
        {
            irrigation_request_t req = { .zone_id = (uint8_t)z, .runtime_s = rt };
            irrigation_start_zone(&req);
        }
    }
    else if (strcmp(cmd, "rain_delay") == 0)
    {
        unsigned h = 0;
        sscanf(payload, "{\"hours\":%u", &h);
        weather_set_rain_delay((uint16_t)h);
    }
}

static void mqtt_event_handler(void *arg, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    (void)arg; (void)base;
    esp_mqtt_event_handle_t ev = (esp_mqtt_event_handle_t)event_data;
    switch ((esp_mqtt_event_id_t)event_id)
    {
        case MQTT_EVENT_CONNECTED:
            s_state = MQTT_MGR_MQTT_CONNECTED;
            ESP_LOGI(TAG, "MQTT connected");
            esp_mqtt_client_subscribe(s_client, TOPIC_CMD_ALL, 1);
            event_bus_publish(EVENT_MQTT_CONNECTED, 0, EVENT_PRIORITY_NORMAL, 0, NULL, 0);
            publish_state();
            break;
        case MQTT_EVENT_DISCONNECTED:
            s_state = MQTT_MGR_WIFI_CONNECTED;
            ESP_LOGW(TAG, "MQTT disconnected");
            event_bus_publish(EVENT_MQTT_DISCONNECTED, 0, EVENT_PRIORITY_NORMAL, 0, NULL, 0);
            break;
        case MQTT_EVENT_DATA:
            if (ev && ev->topic && ev->data) {
                char tbuf[128] = {0}, dbuf[256] = {0};
                int tl = ev->topic_len < 127 ? ev->topic_len : 127;
                int dl = ev->data_len  < 255 ? ev->data_len  : 255;
                memcpy(tbuf, ev->topic, (size_t)tl);
                memcpy(dbuf, ev->data,  (size_t)dl);
                const char *sl = strrchr(tbuf, '/');
                if (sl) handle_command(sl + 1, dbuf);
            }
            break;
        default: break;
    }
}

static void wifi_event_handler(void *arg, esp_event_base_t base,
                                int32_t event_id, void *event_data)
{
    (void)arg; (void)event_data; (void)base;
    if (event_id == WIFI_EVENT_STA_START)
    {
        s_state = MQTT_MGR_WIFI_CONNECTING;
        esp_wifi_connect();
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        s_state = MQTT_MGR_WIFI_DISCONNECTED;
        if (s_wifi_retries++ < WIFI_MAX_RETRY) esp_wifi_connect();
    }
}

static void ip_event_handler(void *arg, esp_event_base_t base,
                              int32_t event_id, void *event_data)
{
    (void)arg; (void)base; (void)event_data;
    if (event_id != IP_EVENT_STA_GOT_IP) return;
    s_state = MQTT_MGR_WIFI_CONNECTED;
    s_wifi_retries = 0;
    ESP_LOGI(TAG, "IP obtained, connecting MQTT");

    config_network_t net;
    config_get_network(&net);
    const char *uri = net.mqtt_broker_uri[0] ? net.mqtt_broker_uri : "mqtt://192.168.10.2:1883";

    esp_mqtt_client_config_t mc = {0};
    mc.broker.address.uri = uri;
    mc.credentials.client_id = "controller_01";
    mc.session.last_will.topic  = TOPIC_STATE;
    mc.session.last_will.msg    = "{\"state\":\"offline\"}";
    mc.session.last_will.qos    = 1;
    mc.session.last_will.retain = 1;
    if (net.mqtt_username[0]) mc.credentials.username = net.mqtt_username;
    if (net.mqtt_password[0]) mc.credentials.authentication.password = net.mqtt_password;

    if (!s_client)
    {
        s_client = esp_mqtt_client_init(&mc);
        if (s_client)
        {
            esp_mqtt_client_register_event(s_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
            s_state = MQTT_MGR_MQTT_CONNECTING;
            esp_mqtt_client_start(s_client);
        }
    }
    else
    {
        s_state = MQTT_MGR_MQTT_CONNECTING;
        esp_mqtt_client_reconnect(s_client);
    }
}

static void mgr_task(void *arg)
{
    (void)arg;
    for (;;)
    {
        vTaskDelay(pdMS_TO_TICKS(TELEMETRY_PERIOD_MS));
        if (s_state == MQTT_MGR_MQTT_CONNECTED) publish_telemetry();
    }
}

bool mqtt_manager_init(void)
{
    if (s_initialized) return true;

    event_bus_subscribe(0xFFFFFFFF, on_bus_event, NULL);

    esp_netif_init();
    esp_event_loop_create_default();
    esp_netif_create_default_wifi_sta();

    esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL);
    esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, ip_event_handler, NULL);

    wifi_init_config_t wcfg = WIFI_INIT_CONFIG_DEFAULT();
    esp_wifi_init(&wcfg);
    esp_wifi_set_mode(WIFI_MODE_STA);

    config_network_t net;
    config_get_network(&net);
    if (net.ssid[0])
    {
        wifi_config_t wc = {0};
        strncpy((char *)wc.sta.ssid, net.ssid, sizeof(wc.sta.ssid) - 1);
        strncpy((char *)wc.sta.password, net.password, sizeof(wc.sta.password) - 1);
        esp_wifi_set_config(WIFI_IF_STA, &wc);
        ESP_LOGI(TAG, "WiFi SSID: %s", net.ssid);
    }
    else
    {
        ESP_LOGW(TAG, "No SSID configured");
    }

    esp_wifi_start();
    xTaskCreate(mgr_task, "mqtt_mgr", 4096, NULL, 5, NULL);

    s_initialized = true;
    ESP_LOGI(TAG, "MQTT Manager initialized");
    return true;
}

bool mqtt_manager_is_connected(void)   { return s_state == MQTT_MGR_MQTT_CONNECTED; }
mqtt_mgr_state_t mqtt_manager_get_state(void) { return s_state; }
bool mqtt_manager_publish(const char *topic, const char *payload, int qos, bool retain)
{
    if (!topic || !payload) return false;
    return safe_publish(topic, payload, qos, retain ? 1 : 0);
}
