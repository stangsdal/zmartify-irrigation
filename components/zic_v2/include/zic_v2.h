/**
 * zic_v2 - Zmartify MQTT v2 contract adapter for the irrigation controller.
 *
 * Provides topic builders and schema-conformant payload builders for the
 * Zmartify Edge v2 contracts:
 *   - contracts/mqtt-v2/reported-state.schema.json (irrigation blocks)
 *   - contracts/mqtt-v2/irrigation-outcome.schema.json
 *
 * Topics:
 *   zmartify/v2/devices/{device_id}/commands/irrigation/#   (subscribe)
 *   zmartify/v2/devices/{device_id}/events/irrigation/outcome (publish)
 *
 * The edge ingests reported state via HTTP POST
 *   /api/v2/devices/{device_id}/ingest/mqtt/reported-state
 * and outcomes via the v2 outcome topic or HTTP
 *   /api/v2/devices/{device_id}/ingest/mqtt/irrigation/outcome
 *
 * Dependency-free (libc only) so it can be adopted incrementally.
 */

#ifndef ZIC_V2_H
#define ZIC_V2_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Build the v2 outcome event topic for this device. */
bool zic_v2_outcome_topic(const char *device_id, char *out, size_t out_len);

/** Build the v2 reported-state topic for this device. */
bool zic_v2_state_topic(const char *device_id, char *out, size_t out_len);

/** Build the v2 command subscription filter (irrigation commands). */
bool zic_v2_command_filter(const char *device_id, char *out, size_t out_len);

typedef struct {
    double flow_lpm;         /* < 0 to omit */
    double pressure_bar;     /* < 0 to omit */
    double water_liters;     /* < 0 to omit */
} zic_v2_hydraulics_t;

typedef struct {
    double voltage_rms_v;    /* < 0 to omit */
    double current_rms_a;    /* < 0 to omit */
    double real_power_w;     /* < 0 to omit */
    double power_factor;     /* < 0 to omit */
} zic_v2_power_t;

typedef struct {
    double temperature_c;    /* use ZIC_V2_OMIT to omit */
    double rain_mm;          /* < 0 to omit */
    double wind_mps;         /* < 0 to omit */
    double eto_mm;           /* < 0 to omit */
} zic_v2_weather_t;

#define ZIC_V2_OMIT (-1000.0)
#define ZIC_V2_COMMAND_ID_MAX 40u
#define ZIC_V2_DEDUPE_CAPACITY 16u

typedef enum {
    ZIC_V2_ACTION_ZONE_START = 0,
    ZIC_V2_ACTION_ZONE_STOP,
    ZIC_V2_ACTION_STOP_ALL,
    ZIC_V2_ACTION_RAIN_DELAY,
} zic_v2_command_action_t;

typedef enum {
    ZIC_V2_COMMAND_ACCEPTED = 0,
    ZIC_V2_COMMAND_REJECTED,
    ZIC_V2_COMMAND_DUPLICATE,
} zic_v2_command_decision_t;

typedef enum {
    ZIC_V2_REASON_NONE = 0,
    ZIC_V2_REASON_UNAUTHORIZED,
    ZIC_V2_REASON_INVALID_ID,
    ZIC_V2_REASON_INVALID_TIMESTAMP,
    ZIC_V2_REASON_STALE,
    ZIC_V2_REASON_FUTURE,
    ZIC_V2_REASON_INVALID_ZONE,
    ZIC_V2_REASON_INVALID_RUNTIME,
    ZIC_V2_REASON_INVALID_RAIN_DELAY,
} zic_v2_command_reason_t;

typedef struct {
    char command_id[ZIC_V2_COMMAND_ID_MAX];
    bool authorized;
    uint32_t source_epoch_s;
    zic_v2_command_action_t action;
    uint32_t zone_id;
    uint32_t runtime_seconds;
    uint32_t rain_delay_hours;
} zic_v2_command_t;

typedef struct {
    char command_ids[ZIC_V2_DEDUPE_CAPACITY][ZIC_V2_COMMAND_ID_MAX];
    uint8_t next_index;
} zic_v2_command_tracker_t;

bool zic_v2_parse_utc_timestamp(const char *timestamp, uint32_t *epoch_s_out);
zic_v2_command_decision_t zic_v2_validate_command(
    const zic_v2_command_t *command,
    uint32_t now_epoch_s,
    zic_v2_command_tracker_t *tracker,
    zic_v2_command_reason_t *reason_out);
const char *zic_v2_command_decision_name(zic_v2_command_decision_t decision);
const char *zic_v2_command_reason_name(zic_v2_command_reason_t reason);

/**
 * Build a schema-conformant v2 reported-state payload with irrigation blocks.
 *
 * @param source_timestamp ISO-8601 UTC timestamp (e.g. "2026-07-12T18:00:00Z")
 * @param firmware_version optional (NULL to omit)
 * @param hydraulics/power/weather optional (NULL to omit block)
 * @return true when the payload fits in the buffer
 */
bool zic_v2_build_reported_state(char *out,
                                 size_t out_len,
                                 const char *source_timestamp,
                                 const char *firmware_version,
                                 const zic_v2_hydraulics_t *hydraulics,
                                 const zic_v2_power_t *power,
                                 const zic_v2_weather_t *weather);

/**
 * Build a schema-conformant v2 irrigation outcome payload.
 *
 * event_type examples: "run.completed", "valve.fault", "pressure.alarm".
 * severity: "info", "warning", "alarm" or "critical".
 * result/detail/run_id/program_id are optional (NULL to omit).
 * zone_id <= 0 omits the field.
 */
bool zic_v2_build_outcome(char *out,
                          size_t out_len,
                          const char *source_timestamp,
                          const char *event_type,
                          const char *severity,
                          const char *result,
                          const char *detail,
                          const char *run_id,
                          const char *program_id,
                          int zone_id);

#ifdef __cplusplus
}
#endif

#endif /* ZIC_V2_H */
