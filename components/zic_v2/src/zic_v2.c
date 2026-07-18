#include "zic_v2.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

static bool zic_v2_appendf(char *buf, size_t buf_len, size_t *pos, const char *fmt, ...)
{
    if (buf == NULL || pos == NULL || *pos >= buf_len) {
        return false;
    }

    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(buf + *pos, buf_len - *pos, fmt, args);
    va_end(args);

    if (written < 0 || (size_t)written >= (buf_len - *pos)) {
        return false;
    }
    *pos += (size_t)written;
    return true;
}

bool zic_v2_outcome_topic(const char *device_id, char *out, size_t out_len)
{
    if (device_id == NULL || out == NULL) {
        return false;
    }
    int written = snprintf(out, out_len, "zmartify/v2/devices/%s/events/irrigation/outcome", device_id);
    return written > 0 && (size_t)written < out_len;
}

bool zic_v2_state_topic(const char *device_id, char *out, size_t out_len)
{
    if (device_id == NULL || out == NULL) {
        return false;
    }
    int written = snprintf(out, out_len, "zmartify/v2/devices/%s/state/reported", device_id);
    return written > 0 && (size_t)written < out_len;
}

bool zic_v2_command_filter(const char *device_id, char *out, size_t out_len)
{
    if (device_id == NULL || out == NULL) {
        return false;
    }
    int written = snprintf(out, out_len, "zmartify/v2/devices/%s/commands/irrigation/#", device_id);
    return written > 0 && (size_t)written < out_len;
}

bool zic_v2_build_reported_state(char *out,
                                 size_t out_len,
                                 const char *source_timestamp,
                                 const char *firmware_version,
                                 const zic_v2_hydraulics_t *hydraulics,
                                 const zic_v2_power_t *power,
                                 const zic_v2_weather_t *weather)
{
    if (out == NULL || out_len == 0 || source_timestamp == NULL) {
        return false;
    }

    size_t pos = 0;
    if (!zic_v2_appendf(out, out_len, &pos,
                        "{\"schema_version\":\"2.0\",\"source_timestamp\":\"%s\"",
                        source_timestamp)) {
        return false;
    }

    if (firmware_version != NULL && firmware_version[0] != '\0') {
        if (!zic_v2_appendf(out, out_len, &pos, ",\"firmware_version\":\"%s\"", firmware_version)) {
            return false;
        }
    }

    if (hydraulics != NULL) {
        if (!zic_v2_appendf(out, out_len, &pos, ",\"hydraulics\":{")) {
            return false;
        }
        bool first = true;
        if (hydraulics->flow_lpm >= 0.0 &&
            !zic_v2_appendf(out, out_len, &pos, "%s\"flow_lpm\":%.2f", first ? "" : ",", hydraulics->flow_lpm)) {
            return false;
        }
        first = first && hydraulics->flow_lpm < 0.0;
        if (hydraulics->pressure_bar >= 0.0 &&
            !zic_v2_appendf(out, out_len, &pos, "%s\"pressure_bar\":%.2f", first ? "" : ",", hydraulics->pressure_bar)) {
            return false;
        }
        first = first && hydraulics->pressure_bar < 0.0;
        if (hydraulics->water_liters >= 0.0 &&
            !zic_v2_appendf(out, out_len, &pos, "%s\"water_liters\":%.1f", first ? "" : ",", hydraulics->water_liters)) {
            return false;
        }
        if (!zic_v2_appendf(out, out_len, &pos, "}")) {
            return false;
        }
    }

    if (power != NULL) {
        if (!zic_v2_appendf(out, out_len, &pos, ",\"power\":{")) {
            return false;
        }
        bool first = true;
        if (power->voltage_rms_v >= 0.0 &&
            !zic_v2_appendf(out, out_len, &pos, "%s\"voltage_rms_v\":%.2f", first ? "" : ",", power->voltage_rms_v)) {
            return false;
        }
        first = first && power->voltage_rms_v < 0.0;
        if (power->current_rms_a >= 0.0 &&
            !zic_v2_appendf(out, out_len, &pos, "%s\"current_rms_a\":%.3f", first ? "" : ",", power->current_rms_a)) {
            return false;
        }
        first = first && power->current_rms_a < 0.0;
        if (power->real_power_w >= 0.0 &&
            !zic_v2_appendf(out, out_len, &pos, "%s\"real_power_w\":%.1f", first ? "" : ",", power->real_power_w)) {
            return false;
        }
        first = first && power->real_power_w < 0.0;
        if (power->power_factor >= 0.0 &&
            !zic_v2_appendf(out, out_len, &pos, "%s\"power_factor\":%.3f", first ? "" : ",", power->power_factor)) {
            return false;
        }
        if (!zic_v2_appendf(out, out_len, &pos, "}")) {
            return false;
        }
    }

    if (weather != NULL) {
        if (!zic_v2_appendf(out, out_len, &pos, ",\"irrigation\":{\"weather\":{")) {
            return false;
        }
        bool first = true;
        if (weather->temperature_c > ZIC_V2_OMIT &&
            !zic_v2_appendf(out, out_len, &pos, "%s\"temperature_c\":%.1f", first ? "" : ",", weather->temperature_c)) {
            return false;
        }
        first = first && weather->temperature_c <= ZIC_V2_OMIT;
        if (weather->rain_mm >= 0.0 &&
            !zic_v2_appendf(out, out_len, &pos, "%s\"rain_mm\":%.2f", first ? "" : ",", weather->rain_mm)) {
            return false;
        }
        first = first && weather->rain_mm < 0.0;
        if (weather->wind_mps >= 0.0 &&
            !zic_v2_appendf(out, out_len, &pos, "%s\"wind_mps\":%.1f", first ? "" : ",", weather->wind_mps)) {
            return false;
        }
        first = first && weather->wind_mps < 0.0;
        if (weather->eto_mm >= 0.0 &&
            !zic_v2_appendf(out, out_len, &pos, "%s\"eto_mm\":%.2f", first ? "" : ",", weather->eto_mm)) {
            return false;
        }
        if (!zic_v2_appendf(out, out_len, &pos, "}}")) {
            return false;
        }
    }

    return zic_v2_appendf(out, out_len, &pos, "}");
}

bool zic_v2_build_outcome(char *out,
                          size_t out_len,
                          const char *source_timestamp,
                          const char *event_type,
                          const char *severity,
                          const char *result,
                          const char *detail,
                          const char *run_id,
                          const char *program_id,
                          int zone_id)
{
    if (out == NULL || out_len == 0 || source_timestamp == NULL || event_type == NULL) {
        return false;
    }

    size_t pos = 0;
    if (!zic_v2_appendf(out, out_len, &pos,
                        "{\"schema_version\":\"2.0\",\"source_timestamp\":\"%s\",\"event_type\":\"%s\"",
                        source_timestamp,
                        event_type)) {
        return false;
    }

    if (severity != NULL && !zic_v2_appendf(out, out_len, &pos, ",\"severity\":\"%s\"", severity)) {
        return false;
    }
    if (result != NULL && !zic_v2_appendf(out, out_len, &pos, ",\"result\":\"%s\"", result)) {
        return false;
    }
    if (detail != NULL && !zic_v2_appendf(out, out_len, &pos, ",\"detail\":\"%.96s\"", detail)) {
        return false;
    }
    if (run_id != NULL && !zic_v2_appendf(out, out_len, &pos, ",\"run_id\":\"%s\"", run_id)) {
        return false;
    }
    if (program_id != NULL && !zic_v2_appendf(out, out_len, &pos, ",\"program_id\":\"%s\"", program_id)) {
        return false;
    }
    if (zone_id > 0 && !zic_v2_appendf(out, out_len, &pos, ",\"zone_id\":%d", zone_id)) {
        return false;
    }

    return zic_v2_appendf(out, out_len, &pos, "}");
}
