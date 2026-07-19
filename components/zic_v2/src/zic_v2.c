#include "zic_v2.h"

#include <stdarg.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define ZIC_V2_COMMAND_MAX_AGE_S 300u
#define ZIC_V2_COMMAND_MAX_FUTURE_S 30u
#define ZIC_V2_MAX_RUNTIME_S 7200u
#define ZIC_V2_MAX_RAIN_DELAY_H 8760u

static bool zic_v2_is_leap_year(unsigned year)
{
    return (year % 4u == 0u && year % 100u != 0u) || year % 400u == 0u;
}

static unsigned zic_v2_days_in_month(unsigned year, unsigned month)
{
    static const uint8_t days[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if (month == 2u && zic_v2_is_leap_year(year)) {
        return 29u;
    }
    return month >= 1u && month <= 12u ? days[month - 1u] : 0u;
}

static int64_t zic_v2_days_from_civil(int year, unsigned month, unsigned day)
{
    year -= month <= 2u;
    const int era = (year >= 0 ? year : year - 399) / 400;
    const unsigned year_of_era = (unsigned)(year - era * 400);
    const unsigned day_of_year = (153u * (month + (month > 2u ? (unsigned)-3 : 9u)) + 2u) / 5u + day - 1u;
    const unsigned day_of_era = year_of_era * 365u + year_of_era / 4u - year_of_era / 100u + day_of_year;
    return era * 146097LL + (int64_t)day_of_era - 719468LL;
}

bool zic_v2_parse_utc_timestamp(const char *timestamp, uint32_t *epoch_s_out)
{
    unsigned year, month, day, hour, minute, second;
    char trailing;
    if (timestamp == NULL || epoch_s_out == NULL || strlen(timestamp) != 20u ||
        sscanf(timestamp, "%4u-%2u-%2uT%2u:%2u:%2uZ%c",
               &year, &month, &day, &hour, &minute, &second, &trailing) != 6 ||
        year < 1970u || year > 2105u || day == 0u ||
        day > zic_v2_days_in_month(year, month) || hour > 23u || minute > 59u || second > 59u) {
        return false;
    }

    int64_t epoch = zic_v2_days_from_civil((int)year, month, day) * 86400LL +
        (int64_t)hour * 3600LL + (int64_t)minute * 60LL + second;
    if (epoch < 0 || epoch > UINT32_MAX) {
        return false;
    }
    *epoch_s_out = (uint32_t)epoch;
    return true;
}

static bool zic_v2_valid_command_id(const char *command_id)
{
    size_t length = strnlen(command_id, ZIC_V2_COMMAND_ID_MAX);
    if (length == 0u || length >= ZIC_V2_COMMAND_ID_MAX) {
        return false;
    }
    for (size_t index = 0; index < length; ++index) {
        unsigned char character = (unsigned char)command_id[index];
        if (!isalnum(character) && character != '-' && character != '_' &&
            character != '.' && character != ':') {
            return false;
        }
    }
    return true;
}

static bool zic_v2_tracker_contains(const zic_v2_command_tracker_t *tracker,
                                    const char *command_id)
{
    for (size_t index = 0; index < ZIC_V2_DEDUPE_CAPACITY; ++index) {
        if (strcmp(tracker->command_ids[index], command_id) == 0) {
            return true;
        }
    }
    return false;
}

zic_v2_command_decision_t zic_v2_validate_command(
    const zic_v2_command_t *command,
    uint32_t now_epoch_s,
    zic_v2_command_tracker_t *tracker,
    zic_v2_command_reason_t *reason_out)
{
    zic_v2_command_reason_t reason = ZIC_V2_REASON_NONE;
    if (command == NULL || tracker == NULL || !zic_v2_valid_command_id(command->command_id)) {
        reason = ZIC_V2_REASON_INVALID_ID;
    } else if (!command->authorized) {
        reason = ZIC_V2_REASON_UNAUTHORIZED;
    } else if (command->source_epoch_s == 0u || now_epoch_s == 0u) {
        reason = ZIC_V2_REASON_INVALID_TIMESTAMP;
    } else if (command->source_epoch_s > now_epoch_s + ZIC_V2_COMMAND_MAX_FUTURE_S) {
        reason = ZIC_V2_REASON_FUTURE;
    } else if (command->source_epoch_s + ZIC_V2_COMMAND_MAX_AGE_S < now_epoch_s) {
        reason = ZIC_V2_REASON_STALE;
    } else if ((command->action == ZIC_V2_ACTION_ZONE_START ||
                command->action == ZIC_V2_ACTION_ZONE_STOP) &&
               (command->zone_id == 0u || command->zone_id > 15u)) {
        reason = ZIC_V2_REASON_INVALID_ZONE;
    } else if (command->action == ZIC_V2_ACTION_ZONE_START &&
               (command->runtime_seconds == 0u || command->runtime_seconds > ZIC_V2_MAX_RUNTIME_S)) {
        reason = ZIC_V2_REASON_INVALID_RUNTIME;
    } else if (command->action == ZIC_V2_ACTION_RAIN_DELAY &&
               command->rain_delay_hours > ZIC_V2_MAX_RAIN_DELAY_H) {
        reason = ZIC_V2_REASON_INVALID_RAIN_DELAY;
    }

    if (reason_out != NULL) {
        *reason_out = reason;
    }
    if (reason != ZIC_V2_REASON_NONE) {
        return ZIC_V2_COMMAND_REJECTED;
    }
    if (zic_v2_tracker_contains(tracker, command->command_id)) {
        return ZIC_V2_COMMAND_DUPLICATE;
    }

    strncpy(tracker->command_ids[tracker->next_index], command->command_id,
            ZIC_V2_COMMAND_ID_MAX - 1u);
    tracker->command_ids[tracker->next_index][ZIC_V2_COMMAND_ID_MAX - 1u] = '\0';
    tracker->next_index = (uint8_t)((tracker->next_index + 1u) % ZIC_V2_DEDUPE_CAPACITY);
    return ZIC_V2_COMMAND_ACCEPTED;
}

const char *zic_v2_command_decision_name(zic_v2_command_decision_t decision)
{
    switch (decision) {
    case ZIC_V2_COMMAND_ACCEPTED: return "accepted";
    case ZIC_V2_COMMAND_DUPLICATE: return "duplicate";
    case ZIC_V2_COMMAND_REJECTED:
    default: return "rejected";
    }
}

const char *zic_v2_command_reason_name(zic_v2_command_reason_t reason)
{
    switch (reason) {
    case ZIC_V2_REASON_NONE: return "none";
    case ZIC_V2_REASON_UNAUTHORIZED: return "unauthorized";
    case ZIC_V2_REASON_INVALID_ID: return "invalid_command_id";
    case ZIC_V2_REASON_INVALID_TIMESTAMP: return "invalid_timestamp";
    case ZIC_V2_REASON_STALE: return "stale_command";
    case ZIC_V2_REASON_FUTURE: return "future_command";
    case ZIC_V2_REASON_INVALID_ZONE: return "invalid_zone";
    case ZIC_V2_REASON_INVALID_RUNTIME: return "invalid_runtime";
    case ZIC_V2_REASON_INVALID_RAIN_DELAY: return "invalid_rain_delay";
    default: return "invalid_command";
    }
}

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
