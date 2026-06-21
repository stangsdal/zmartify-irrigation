#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ZIC_MQTT_ROOT "zmartify/irrigation/controller_01"
#define ZIC_MQTT_TOPIC_MAX 128

typedef enum {
    ZIC_MQTT_QOS_TELEMETRY = 0,
    ZIC_MQTT_QOS_COMMAND = 1,
    ZIC_MQTT_QOS_EVENT = 1
} zic_mqtt_qos_t;

typedef enum {
    ZIC_MQTT_CMD_START_ZONE = 0,
    ZIC_MQTT_CMD_STOP_ZONE,
    ZIC_MQTT_CMD_RUN_PROGRAM,
    ZIC_MQTT_CMD_STOP_ALL,
    ZIC_MQTT_CMD_RAIN_DELAY
} zic_mqtt_command_t;

typedef struct {
    zic_mqtt_command_t command;
    uint8_t zone_id;
    uint32_t runtime_seconds;
    uint16_t rain_delay_hours;
} zic_mqtt_command_payload_t;

typedef struct {
    uint32_t timestamp;
    char event_name[32];
    char severity[16];
    char details[96];
} zic_mqtt_event_payload_t;

bool zic_mqtt_topic_build(const char *suffix, char *out_topic, size_t out_topic_len);
const char *zic_mqtt_command_topic(zic_mqtt_command_t command);
