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
