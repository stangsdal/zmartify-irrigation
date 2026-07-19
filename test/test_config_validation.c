#include <assert.h>
#include <string.h>

#include "config_validation.h"

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
} alarms_v1_t;

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
} zone_v1_t;

static zic_config_t valid_config(void)
{
    zic_config_t config;
    memset(&config, 0, sizeof(config));
    config.system.global_max_runtime_s = 3600;
    config.hydraulics.flow_pulses_per_litre = 450.0f;
    config.hydraulics.pressure_mv_per_bar = 400.0f;
    config.hydraulics.pressure_offset_mv = 500.0f;
    config.hydraulics.pressure_min_bar = 0.5f;
    config.hydraulics.pressure_max_bar = 8.0f;
    config.hydraulics.valve_open_timeout_s = 30;
    config.hydraulics.valve_close_timeout_s = 10;
    config.alarms.pressure_low_mbar = 500;
    config.alarms.pressure_high_mbar = 7000;
    config.alarms.flow_low_lpm_x10 = 20;
    config.alarms.flow_high_lpm_x10 = 2000;
    config.alarms.no_flow_timeout_s = 30;
    config.alarms.high_flow_duration_s = 10;
    config.alarms.pressure_critical_duration_s = 5;
    config.alarms.flow_active_max_age_ms = 1500;
    config.alarms.flow_idle_max_age_ms = 5000;
    config.alarms.cabinet_warn_temp_c = 45;
    config.alarms.cabinet_crit_temp_c = 55;
    for (uint8_t index = 0; index < CONFIG_MAX_ZONES; ++index) {
        config.zones[index].relay_index = index + 1;
        config.zones[index].default_runtime_s = 600;
        config.zones[index].max_runtime_s = 3600;
        config.zones[index].flow_baseline_lpm_x10 = 120;
        config.zones[index].flow_warning_deviation_pct = 15;
        config.zones[index].flow_critical_deviation_pct = 30;
        config.zones[index].pressure_min_mbar = 1000;
        config.zones[index].pressure_max_mbar = 5000;
    }
    return config;
}

static void test_safety_boundaries(void)
{
    zic_config_t config = valid_config();
    assert(config_validate_safety(&config));

    config.alarms.flow_low_lpm_x10 = config.alarms.flow_high_lpm_x10;
    assert(!config_validate_safety(&config));
    config = valid_config();
    config.alarms.flow_idle_max_age_ms = config.alarms.flow_active_max_age_ms - 1;
    assert(!config_validate_safety(&config));
    config = valid_config();
    config.alarms.pressure_critical_duration_s = 0;
    assert(!config_validate_safety(&config));
    config = valid_config();
    config.zones[0].max_runtime_s = config.system.global_max_runtime_s + 1;
    assert(!config_validate_safety(&config));
    config = valid_config();
    config.zones[0].flow_baseline_lpm_x10 = 3000;
    assert(!config_validate_safety(&config));
    config = valid_config();
    config.zones[0].flow_warning_deviation_pct = 30;
    config.zones[0].flow_critical_deviation_pct = 30;
    assert(!config_validate_safety(&config));
    config = valid_config();
    config.hydraulics.pressure_mv_per_bar = 0.0f;
    assert(!config_validate_safety(&config));
    config = valid_config();
    config.hydraulics.valve_close_timeout_s = 0;
    assert(!config_validate_safety(&config));
}

static void test_zone_commissioning(void)
{
    zic_config_t config = valid_config();
    config_zone_t zone = config.zones[0];
    assert(config_apply_zone_commissioning(&zone, 185, 3000));
    assert(zone.flow_baseline_lpm_x10 == 185);
    assert(zone.pressure_min_mbar == 2400);
    assert(zone.pressure_max_mbar == 3600);
    assert(config_validate_zone_safety(&zone, &config.system));
    assert(!config_apply_zone_commissioning(&zone, 0, 3000));
    assert(!config_apply_zone_commissioning(&zone, 185, 10000));
}

static void test_schema_v1_migration(void)
{
    zic_config_t config = valid_config();
    memset(&config.hydraulics.valve_open_timeout_s, 0, 4);
    alarms_v1_t alarms = {
        .pressure_low_mbar = 500,
        .pressure_high_mbar = 7000,
        .flow_high_lpm_x10 = 2000,
        .flow_low_lpm_x10 = 20,
        .no_flow_timeout_s = 30,
        .high_flow_duration_s = 10,
        .cabinet_warn_temp_c = 47,
        .cabinet_crit_temp_c = 57,
    };
    memcpy(&config.alarms, &alarms, sizeof(alarms));
    for (uint8_t index = 0; index < CONFIG_MAX_ZONES; ++index) {
        zone_v1_t zone;
        memcpy(&zone, &config.zones[index], sizeof(zone));
        zone.seasonal_factor_pct = 95;
        zone.et_crop_coefficient_x100 = 75;
        memset(zone.reserved, 0, sizeof(zone.reserved));
        memcpy(&config.zones[index], &zone, sizeof(zone));
    }
    config.schema_version = 1;

    assert(config_migrate_v1(&config));
    assert(config.schema_version == 2u);
    assert(config.alarms.pressure_critical_duration_s == 5);
    assert(config.alarms.flow_active_max_age_ms == 1500);
    assert(config.alarms.cabinet_warn_temp_c == 47);
    assert(config.hydraulics.valve_open_timeout_s == 0);
    assert(config.hydraulics.valve_close_timeout_s == 0);
    assert(config.zones[0].flow_warning_deviation_pct == 15);
    assert(config.zones[0].flow_critical_deviation_pct == 30);
    assert(config.zones[0].seasonal_factor_pct == 95);
    assert(config.zones[0].et_crop_coefficient_x100 == 75);
    assert(config_migrate_v2(&config));
    assert(config.schema_version == CONFIG_SCHEMA_VERSION);
    assert(config.hydraulics.valve_open_timeout_s == 30);
    assert(config.hydraulics.valve_close_timeout_s == 10);
    assert(config_validate_safety(&config));
    assert(!config_migrate_v1(&config));
}

static void test_schema_v2_migration(void)
{
    zic_config_t config = valid_config();
    memset(&config.hydraulics.valve_open_timeout_s, 0, 4);
    config.schema_version = 2;

    assert(config_migrate_v2(&config));
    assert(config.schema_version == CONFIG_SCHEMA_VERSION);
    assert(config.hydraulics.valve_open_timeout_s == 30);
    assert(config.hydraulics.valve_close_timeout_s == 10);
    assert(config_validate_safety(&config));
    assert(!config_migrate_v2(&config));
}

static void test_atomic_migration_dispatch(void)
{
    zic_config_t config = valid_config();
    config.schema_version = 2u;
    memset(&config.hydraulics.valve_open_timeout_s, 0, 4);
    assert(config_migrate_to_current(&config));
    assert(config.schema_version == CONFIG_SCHEMA_VERSION);
    assert(config.hydraulics.valve_open_timeout_s == 30u);

    config = valid_config();
    config.schema_version = 99u;
    zic_config_t original = config;
    assert(!config_migrate_to_current(&config));
    assert(memcmp(&config, &original, sizeof(config)) == 0);

    config = valid_config();
    config.schema_version = 2u;
    config.alarms.pressure_low_mbar = config.alarms.pressure_high_mbar;
    original = config;
    assert(!config_migrate_to_current(&config));
    assert(memcmp(&config, &original, sizeof(config)) == 0);
}

int main(void)
{
    test_safety_boundaries();
    test_zone_commissioning();
    test_schema_v1_migration();
    test_schema_v2_migration();
    test_atomic_migration_dispatch();
    return 0;
}