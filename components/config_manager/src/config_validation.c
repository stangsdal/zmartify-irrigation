#include "config_validation.h"

#include <math.h>
#include <stddef.h>
#include <string.h>

#define CONFIG_FLOW_MAX_LPM_X10          5000u
#define CONFIG_PRESSURE_MAX_MBAR         10000u
#define CONFIG_RUNTIME_MAX_S             86400u
#define CONFIG_SUPERVISION_MAX_S         300u
#define CONFIG_ACTIVE_MAX_AGE_MIN_MS     250u
#define CONFIG_ACTIVE_MAX_AGE_MAX_MS     5000u
#define CONFIG_IDLE_MAX_AGE_MAX_MS       60000u

typedef struct
{
    uint16_t pressure_low_mbar;
    uint16_t pressure_high_mbar;
    uint16_t flow_high_lpm_x10;
    uint16_t flow_low_lpm_x10;
    uint32_t no_flow_timeout_s;
    uint32_t high_flow_duration_s;
    uint8_t cabinet_warn_temp_c;
    uint8_t cabinet_crit_temp_c;
    uint8_t reserved[6];
} config_alarms_v1_t;

typedef struct
{
    char name[CONFIG_ZONE_NAME_LEN];
    uint8_t relay_index;
    bool enabled;
    config_plant_type_t plant_type;
    config_soil_type_t soil_type;
    config_emitter_type_t emitter_type;
    uint16_t area_m2;
    uint32_t default_runtime_s;
    uint32_t max_runtime_s;
    uint16_t flow_baseline_lpm_x10;
    uint16_t pressure_min_mbar;
    uint16_t pressure_max_mbar;
    uint8_t seasonal_factor_pct;
    uint8_t et_crop_coefficient_x100;
    uint8_t reserved[6];
} config_zone_v1_t;

_Static_assert(sizeof(config_alarms_v1_t) == sizeof(config_alarms_t),
               "schema v1 alarm layout size changed");
_Static_assert(sizeof(config_zone_v1_t) == sizeof(config_zone_t),
               "schema v1 zone layout size changed");

bool config_validate_hydraulics(const config_hydraulic_t *config)
{
    return config != NULL &&
        isfinite(config->flow_pulses_per_litre) && config->flow_pulses_per_litre > 0.0f &&
        isfinite(config->pressure_mv_per_bar) && config->pressure_mv_per_bar > 0.0f &&
        isfinite(config->pressure_offset_mv) && config->pressure_offset_mv >= 0.0f &&
        isfinite(config->pressure_min_bar) && config->pressure_min_bar >= 0.0f &&
        isfinite(config->pressure_max_bar) &&
        config->pressure_max_bar > config->pressure_min_bar &&
        config->pressure_max_bar <= 10.0f &&
        config->valve_open_timeout_s > 0u &&
        config->valve_open_timeout_s <= CONFIG_SUPERVISION_MAX_S &&
        config->valve_close_timeout_s > 0u &&
        config->valve_close_timeout_s <= CONFIG_SUPERVISION_MAX_S;
}

bool config_validate_alarms(const config_alarms_t *config)
{
    return config != NULL &&
        config->pressure_low_mbar > 0u &&
        config->pressure_low_mbar < config->pressure_high_mbar &&
        config->pressure_high_mbar <= CONFIG_PRESSURE_MAX_MBAR &&
        config->flow_low_lpm_x10 > 0u &&
        config->flow_low_lpm_x10 < config->flow_high_lpm_x10 &&
        config->flow_high_lpm_x10 <= CONFIG_FLOW_MAX_LPM_X10 &&
        config->no_flow_timeout_s > 0u &&
        config->no_flow_timeout_s <= CONFIG_SUPERVISION_MAX_S &&
        config->high_flow_duration_s > 0u &&
        config->high_flow_duration_s <= CONFIG_SUPERVISION_MAX_S &&
        config->pressure_critical_duration_s > 0u &&
        config->pressure_critical_duration_s <= CONFIG_SUPERVISION_MAX_S &&
        config->flow_active_max_age_ms >= CONFIG_ACTIVE_MAX_AGE_MIN_MS &&
        config->flow_active_max_age_ms <= CONFIG_ACTIVE_MAX_AGE_MAX_MS &&
        config->flow_idle_max_age_ms >= config->flow_active_max_age_ms &&
        config->flow_idle_max_age_ms <= CONFIG_IDLE_MAX_AGE_MAX_MS &&
        config->cabinet_warn_temp_c < config->cabinet_crit_temp_c;
}

bool config_validate_zone_safety(const config_zone_t *zone,
                                 const config_system_t *system)
{
    return zone != NULL && system != NULL &&
        zone->relay_index > 0u && zone->relay_index <= CONFIG_MAX_ZONES &&
        zone->default_runtime_s > 0u &&
        zone->default_runtime_s <= zone->max_runtime_s &&
        zone->max_runtime_s <= system->global_max_runtime_s &&
        zone->max_runtime_s <= CONFIG_RUNTIME_MAX_S &&
        zone->flow_baseline_lpm_x10 > 0u &&
        zone->flow_baseline_lpm_x10 <= CONFIG_FLOW_MAX_LPM_X10 &&
        zone->flow_warning_deviation_pct > 0u &&
        zone->flow_warning_deviation_pct < zone->flow_critical_deviation_pct &&
        zone->flow_critical_deviation_pct <= 100u &&
        zone->pressure_min_mbar > 0u &&
        zone->pressure_min_mbar < zone->pressure_max_mbar &&
        zone->pressure_max_mbar <= CONFIG_PRESSURE_MAX_MBAR;
}

bool config_validate_safety(const zic_config_t *config)
{
    if (config == NULL || config->system.global_max_runtime_s == 0u ||
        config->system.global_max_runtime_s > CONFIG_RUNTIME_MAX_S ||
        !config_validate_hydraulics(&config->hydraulics) ||
        !config_validate_alarms(&config->alarms)) {
        return false;
    }

    for (uint8_t index = 0; index < CONFIG_MAX_ZONES; ++index) {
        if (!config_validate_zone_safety(&config->zones[index], &config->system)) {
            return false;
        }
        uint32_t baseline = config->zones[index].flow_baseline_lpm_x10;
        uint32_t delta = (baseline * config->zones[index].flow_critical_deviation_pct) / 100u;
        uint32_t zone_low = baseline - delta;
        uint32_t zone_high = baseline + delta;
        uint32_t effective_low = zone_low > config->alarms.flow_low_lpm_x10
            ? zone_low : config->alarms.flow_low_lpm_x10;
        uint32_t effective_high = zone_high < config->alarms.flow_high_lpm_x10
            ? zone_high : config->alarms.flow_high_lpm_x10;
        if (effective_low >= effective_high) {
            return false;
        }
    }
    return true;
}

bool config_migrate_v1(zic_config_t *config)
{
    if (config == NULL || config->schema_version != 1u) {
        return false;
    }

    config_alarms_v1_t legacy_alarms;
    memcpy(&legacy_alarms, &config->alarms, sizeof(legacy_alarms));
    config->alarms.pressure_critical_duration_s = 5;
    config->alarms.flow_active_max_age_ms = 1500;
    config->alarms.flow_idle_max_age_ms = 5000;
    config->alarms.cabinet_warn_temp_c = legacy_alarms.cabinet_warn_temp_c;
    config->alarms.cabinet_crit_temp_c = legacy_alarms.cabinet_crit_temp_c;
    for (uint8_t index = 0; index < CONFIG_MAX_ZONES; ++index) {
        config_zone_v1_t legacy_zone;
        memcpy(&legacy_zone, &config->zones[index], sizeof(legacy_zone));
        config->zones[index].flow_warning_deviation_pct = 15;
        config->zones[index].flow_critical_deviation_pct = 30;
        config->zones[index].seasonal_factor_pct = legacy_zone.seasonal_factor_pct;
        config->zones[index].et_crop_coefficient_x100 =
            legacy_zone.et_crop_coefficient_x100;
    }
    config->schema_version = 2u;
    return true;
}

bool config_migrate_v2(zic_config_t *config)
{
    if (config == NULL || config->schema_version != 2u) {
        return false;
    }
    config->hydraulics.valve_open_timeout_s = 30;
    config->hydraulics.valve_close_timeout_s = 10;
    config->schema_version = CONFIG_SCHEMA_VERSION;
    return true;
}

bool config_migrate_to_current(zic_config_t *config)
{
    if (config == NULL) {
        return false;
    }

    zic_config_t candidate = *config;
    while (candidate.schema_version != CONFIG_SCHEMA_VERSION) {
        bool migrated = false;
        switch (candidate.schema_version) {
        case 1u:
            migrated = config_migrate_v1(&candidate);
            break;
        case 2u:
            migrated = config_migrate_v2(&candidate);
            break;
        default:
            return false;
        }
        if (!migrated) {
            return false;
        }
    }
    if (!config_validate_safety(&candidate)) {
        return false;
    }
    *config = candidate;
    return true;
}

bool config_apply_zone_commissioning(config_zone_t *zone,
                                     uint16_t observed_flow_lpm_x10,
                                     uint16_t observed_pressure_mbar)
{
    if (zone == NULL || observed_flow_lpm_x10 == 0u ||
        observed_flow_lpm_x10 > CONFIG_FLOW_MAX_LPM_X10 ||
        observed_pressure_mbar < 200u || observed_pressure_mbar > CONFIG_PRESSURE_MAX_MBAR) {
        return false;
    }

    uint32_t pressure_margin = observed_pressure_mbar / 5u;
    uint32_t pressure_min = observed_pressure_mbar - pressure_margin;
    uint32_t pressure_max = observed_pressure_mbar + pressure_margin;
    if (pressure_min == 0u || pressure_max > CONFIG_PRESSURE_MAX_MBAR) {
        return false;
    }

    zone->flow_baseline_lpm_x10 = observed_flow_lpm_x10;
    zone->pressure_min_mbar = (uint16_t)pressure_min;
    zone->pressure_max_mbar = (uint16_t)pressure_max;
    return true;
}