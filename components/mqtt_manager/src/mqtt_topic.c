#include "mqtt_topic.h"

#include <stdio.h>
#include <string.h>

bool zic_mqtt_topic_build(const char *suffix, char *out_topic, size_t out_topic_len)
{
    if (suffix == 0 || out_topic == 0 || out_topic_len == 0) {
        return false;
    }

    int written = snprintf(out_topic, out_topic_len, "%s/%s", ZIC_MQTT_ROOT, suffix);
    if (written < 0 || (size_t)written >= out_topic_len) {
        return false;
    }

    return true;
}

const char *zic_mqtt_command_topic(zic_mqtt_command_t command)
{
    switch (command) {
    case ZIC_MQTT_CMD_START_ZONE:
        return ZIC_MQTT_ROOT "/command/start_zone";
    case ZIC_MQTT_CMD_STOP_ZONE:
        return ZIC_MQTT_ROOT "/command/stop_zone";
    case ZIC_MQTT_CMD_RUN_PROGRAM:
        return ZIC_MQTT_ROOT "/command/run_program";
    case ZIC_MQTT_CMD_STOP_ALL:
        return ZIC_MQTT_ROOT "/command/stop_all";
    case ZIC_MQTT_CMD_RAIN_DELAY:
        return ZIC_MQTT_ROOT "/command/rain_delay";
    default:
        return ZIC_MQTT_ROOT "/command/unknown";
    }
}
