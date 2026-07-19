/*
 * config_manager.c
 * Configuration Manager – centralised persistent config
 *
 * Storage layout:
 *   NVS namespace: "zic_config"
 *   Key "cfg_blob" : serialised zic_config_t (with CRC32 header)
 *
 * CRC32 covers all bytes in zic_config_t starting after the crc32 field.
 * Corruption is detected by re-computing the CRC on load.
 */

#include "config_manager.h"
#include "config_validation.h"
#include "hal.h"
#include "event_bus.h"
#include "esp_log.h"
#include "esp_rom_crc.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include <string.h>

static const char *TAG = "cfg_mgr";

/* NVS key for the configuration blob */
#define CFG_NVS_KEY  "cfg_blob"

/* ─── Module state ──────────────────────────────────────────────────── */

static zic_config_t     s_config;
static SemaphoreHandle_t s_lock       = NULL;
static bool              s_initialized = false;
static bool              s_dirty       = false;
static bool              s_migrated    = false;

/* ─── CRC helpers ───────────────────────────────────────────────────── */

/**
 * Compute CRC32 over all bytes of zic_config_t that follow the crc32 field.
 * The fields covered are: everything from `schema_version` onwards but we
 * skip magic(2) + crc32(4) = first 6 bytes.
 */
static uint32_t compute_crc(const zic_config_t *cfg)
{
    /* Start computing from `schema_version` field (offset of schema_version) */
    const uint8_t *start = (const uint8_t *)&cfg->schema_version;
    size_t len = sizeof(zic_config_t) - offsetof(zic_config_t, schema_version);
    return esp_rom_crc32_le(0, start, (uint32_t)len);
}

/* ─── Factory defaults ───────────────────────────────────────────────── */

static void apply_factory_defaults(zic_config_t *cfg)
{
    memset(cfg, 0, sizeof(zic_config_t));

    cfg->magic          = CONFIG_MAGIC;
    cfg->schema_version = CONFIG_SCHEMA_VERSION;

    /* System */
    cfg->system.controller_id          = 0x00000001;
    cfg->system.operational_mode       = CONFIG_MODE_AUTO;
    cfg->system.active_zone_count      = CONFIG_MAX_ZONES;
    cfg->system.max_simultaneous_zones = 1;
    cfg->system.global_max_runtime_s   = 3600;  /* 1 hour */
    cfg->system.master_valve_enabled   = true;

    /* Network */
    strncpy(cfg->network.mqtt_broker_uri, "mqtt://192.168.10.2:1883",
            CONFIG_MQTT_URI_LEN - 1);
    strncpy(cfg->network.ntp_server, "pool.ntp.org", CONFIG_NTP_LEN - 1);
    strncpy(cfg->network.timezone, "UTC0", CONFIG_TZ_LEN - 1);
    cfg->network.mqtt_port = 1883;
    cfg->network.wifi_max_retries = 5;

    /* Hydraulics – typical values, calibrated per installation */
    cfg->hydraulics.flow_pulses_per_litre = 450.0f;   /* DN50 G2 typical */
    cfg->hydraulics.pressure_mv_per_bar   = 400.0f;   /* 0.5-4.5 V → 0-10 bar */
    cfg->hydraulics.pressure_offset_mv    = 500.0f;   /* 0.5 V = 0 bar */
    cfg->hydraulics.pressure_min_bar      = 0.5f;
    cfg->hydraulics.pressure_max_bar      = 8.0f;

    /* Alarms */
    cfg->alarms.pressure_low_mbar         = 500;    /* 0.5 bar */
    cfg->alarms.pressure_high_mbar        = 7000;   /* 7 bar */
    cfg->alarms.flow_high_lpm_x10         = 2000;   /* 200 L/min */
    cfg->alarms.flow_low_lpm_x10          = 20;     /* 2 L/min when open */
    cfg->alarms.no_flow_timeout_s         = 30;
    cfg->alarms.high_flow_duration_s      = 10;
    cfg->alarms.pressure_critical_duration_s = 5;
    cfg->alarms.flow_active_max_age_ms    = 1500;
    cfg->alarms.flow_idle_max_age_ms      = 5000;
    cfg->alarms.cabinet_warn_temp_c       = 45;
    cfg->alarms.cabinet_crit_temp_c       = 55;

    /* Zones – sensible defaults for 15 zones */
    for (int i = 0; i < CONFIG_MAX_ZONES; i++)
    {
        snprintf(cfg->zones[i].name, CONFIG_ZONE_NAME_LEN, "Zone %d", i + 1);
        cfg->zones[i].relay_index              = (uint8_t)(i + 1); /* relay 1-15 */
        cfg->zones[i].enabled                  = (i < 8);           /* first 8 enabled */
        cfg->zones[i].plant_type               = CONFIG_PLANT_LAWN;
        cfg->zones[i].soil_type                = CONFIG_SOIL_LOAM;
        cfg->zones[i].emitter_type             = CONFIG_EMITTER_ROTOR;
        cfg->zones[i].area_m2                  = 50;
        cfg->zones[i].default_runtime_s        = 600;  /* 10 minutes */
        cfg->zones[i].max_runtime_s            = 3600; /* 60 minutes hard cap */
        cfg->zones[i].flow_baseline_lpm_x10    = 120;  /* 12.0 L/min */
        cfg->zones[i].pressure_min_mbar        = 1000; /* 1.0 bar */
        cfg->zones[i].pressure_max_mbar        = 5000; /* 5.0 bar */
        cfg->zones[i].flow_warning_deviation_pct = 15;
        cfg->zones[i].flow_critical_deviation_pct = 30;
        cfg->zones[i].seasonal_factor_pct      = 100;
        cfg->zones[i].et_crop_coefficient_x100 = 80;   /* Kc = 0.80 lawn */
    }

    /* Programs – create one default program (Mon/Wed/Fri, 06:00, all zones 10 min) */
    strncpy(cfg->programs[0].name, "Default Program", CONFIG_PROGRAM_NAME_LEN - 1);
    cfg->programs[0].enabled                 = true;
    cfg->programs[0].run_days                = CONFIG_DAY_MON | CONFIG_DAY_WED | CONFIG_DAY_FRI;
    cfg->programs[0].start_times[0].enabled  = true;
    cfg->programs[0].start_times[0].hour     = 6;
    cfg->programs[0].start_times[0].minute   = 0;
    cfg->programs[0].weather_skip_enabled    = true;
    cfg->programs[0].rain_skip_threshold_pct = 50;
    cfg->programs[0].seasonal_adjust_pct     = 100;
    for (int i = 0; i < CONFIG_MAX_ZONES; i++)
    {
        cfg->programs[0].zone_runtime_min[i] = 10;  /* 10 minutes per zone */
    }
}

/* ─── Load / save ─────────────────────────────────────────────────────── */

static cfg_result_t load_from_nvs(void)
{
    size_t len = sizeof(zic_config_t);
    hal_result_t r = hal_storage_read_blob(CFG_NVS_KEY, &s_config, &len);

    if (r != HAL_OK || len != sizeof(zic_config_t))
    {
        ESP_LOGW(TAG, "Config blob not found or wrong size – applying factory defaults");
        return CFG_STORAGE_ERROR;
    }

    /* Sanity check: magic */
    if (s_config.magic != CONFIG_MAGIC)
    {
        ESP_LOGW(TAG, "Config magic mismatch (0x%04X) – resetting", s_config.magic);
        return CFG_CRC_ERROR;
    }

    /* CRC32 integrity */
    uint32_t expected_crc = compute_crc(&s_config);
    if (s_config.crc32 != expected_crc)
    {
        ESP_LOGW(TAG, "Config CRC mismatch (stored=0x%08lX, calc=0x%08lX) – resetting",
                 (unsigned long)s_config.crc32, (unsigned long)expected_crc);
        return CFG_CRC_ERROR;
    }

    if (s_config.schema_version == 1u)
    {
        if (!config_migrate_v1(&s_config))
        {
            return CFG_VERSION_MISMATCH;
        }
        s_migrated = true;
        ESP_LOGI(TAG, "Config migrated from schema v1 to v%u", CONFIG_SCHEMA_VERSION);
    }
    else if (s_config.schema_version != CONFIG_SCHEMA_VERSION)
    {
        ESP_LOGW(TAG, "Config schema %u, current %u – resetting",
                 s_config.schema_version, CONFIG_SCHEMA_VERSION);
        return CFG_VERSION_MISMATCH;
    }

    if (!config_validate_safety(&s_config))
    {
        ESP_LOGW(TAG, "Config safety validation failed – resetting");
        return CFG_VALIDATION_ERROR;
    }

    ESP_LOGI(TAG, "Config loaded from NVS (schema v%u, CRC OK)", s_config.schema_version);
    return CFG_OK;
}

static cfg_result_t save_to_nvs(void)
{
    s_config.magic          = CONFIG_MAGIC;
    s_config.schema_version = CONFIG_SCHEMA_VERSION;
    s_config.crc32          = compute_crc(&s_config);

    hal_result_t r = hal_storage_write_blob(CFG_NVS_KEY, &s_config, sizeof(zic_config_t));
    if (r != HAL_OK)
    {
        ESP_LOGE(TAG, "Failed to write config to NVS");
        return CFG_STORAGE_ERROR;
    }

    ESP_LOGI(TAG, "Config committed to NVS (%u bytes, CRC=0x%08lX)",
             (unsigned)sizeof(zic_config_t), (unsigned long)s_config.crc32);
    return CFG_OK;
}

/* ─── Public API ──────────────────────────────────────────────────────── */

cfg_result_t config_manager_init(void)
{
    if (s_initialized)
    {
        return CFG_OK;
    }

    s_lock = xSemaphoreCreateMutex();
    if (s_lock == NULL)
    {
        ESP_LOGE(TAG, "Failed to create config mutex");
        return CFG_STORAGE_ERROR;
    }

    cfg_result_t r = load_from_nvs();
    if (r != CFG_OK)
    {
        /* Corruption or first boot – apply factory defaults */
        apply_factory_defaults(&s_config);
        save_to_nvs();
        ESP_LOGI(TAG, "Factory defaults applied and committed");
    }
    else if (s_migrated)
    {
        r = save_to_nvs();
        if (r != CFG_OK)
        {
            return r;
        }
        ESP_LOGI(TAG, "Migrated configuration committed");
    }

    s_dirty       = false;
    s_initialized = true;
    ESP_LOGI(TAG, "Configuration Manager ready (schema v%u)", s_config.schema_version);
    return CFG_OK;
}

cfg_result_t config_manager_commit(void)
{
    if (!s_initialized)
    {
        return CFG_NOT_INITIALIZED;
    }

    xSemaphoreTake(s_lock, portMAX_DELAY);
    if (!config_validate_safety(&s_config))
    {
        xSemaphoreGive(s_lock);
        ESP_LOGW(TAG, "Config commit rejected by safety validation");
        return CFG_VALIDATION_ERROR;
    }
    cfg_result_t r = save_to_nvs();
    if (r == CFG_OK)
    {
        s_dirty = false;
        /* Publish config-changed event */
        event_bus_publish(EVENT_SYSTEM_FAULT,   /* placeholder – will be EVENT_CONFIG_CHANGED in Step 6 */
                          0, EVENT_PRIORITY_NORMAL, 0, NULL, 0);
    }
    xSemaphoreGive(s_lock);
    return r;
}

cfg_result_t config_manager_restore_defaults(void)
{
    if (!s_initialized)
    {
        return CFG_NOT_INITIALIZED;
    }

    xSemaphoreTake(s_lock, portMAX_DELAY);
    apply_factory_defaults(&s_config);
    cfg_result_t r = save_to_nvs();
    s_dirty = false;
    xSemaphoreGive(s_lock);

    ESP_LOGW(TAG, "Factory reset complete");
    return r;
}

const zic_config_t *config_get(void)
{
    return &s_config;
}

cfg_result_t config_copy(zic_config_t *dst)
{
    if (!s_initialized || dst == NULL)
    {
        return CFG_NOT_INITIALIZED;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    memcpy(dst, &s_config, sizeof(zic_config_t));
    xSemaphoreGive(s_lock);
    return CFG_OK;
}

/* ─── System ──────────────────────────────────────────────────────────── */

cfg_result_t config_get_system(config_system_t *out)
{
    if (!s_initialized || out == NULL)
    {
        return CFG_NOT_INITIALIZED;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    memcpy(out, &s_config.system, sizeof(config_system_t));
    xSemaphoreGive(s_lock);
    return CFG_OK;
}

cfg_result_t config_set_system(const config_system_t *in)
{
    if (!s_initialized || in == NULL)
    {
        return CFG_NOT_INITIALIZED;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    zic_config_t candidate = s_config;
    candidate.system = *in;
    if (!config_validate_safety(&candidate))
    {
        xSemaphoreGive(s_lock);
        return CFG_VALIDATION_ERROR;
    }
    ESP_LOGI(TAG, "System safety changed: max_runtime_s %lu -> %lu",
             (unsigned long)s_config.system.global_max_runtime_s,
             (unsigned long)in->global_max_runtime_s);
    memcpy(&s_config.system, in, sizeof(config_system_t));
    s_dirty = true;
    xSemaphoreGive(s_lock);
    return CFG_OK;
}

/* ─── Network ─────────────────────────────────────────────────────────── */

cfg_result_t config_get_network(config_network_t *out)
{
    if (!s_initialized || out == NULL)
    {
        return CFG_NOT_INITIALIZED;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    memcpy(out, &s_config.network, sizeof(config_network_t));
    xSemaphoreGive(s_lock);
    return CFG_OK;
}

cfg_result_t config_set_network(const config_network_t *in)
{
    if (!s_initialized || in == NULL)
    {
        return CFG_NOT_INITIALIZED;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    memcpy(&s_config.network, in, sizeof(config_network_t));
    s_dirty = true;
    xSemaphoreGive(s_lock);
    return CFG_OK;
}

/* ─── Hydraulics ──────────────────────────────────────────────────────── */

cfg_result_t config_get_hydraulics(config_hydraulic_t *out)
{
    if (!s_initialized || out == NULL)
    {
        return CFG_NOT_INITIALIZED;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    memcpy(out, &s_config.hydraulics, sizeof(config_hydraulic_t));
    xSemaphoreGive(s_lock);
    return CFG_OK;
}

cfg_result_t config_set_hydraulics(const config_hydraulic_t *in)
{
    if (!s_initialized || in == NULL)
    {
        return CFG_NOT_INITIALIZED;
    }
    if (!config_validate_hydraulics(in))
    {
        return CFG_VALIDATION_ERROR;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    ESP_LOGI(TAG, "Hydraulic calibration changed: pressure_mv_per_bar %.2f -> %.2f, offset_mv %.2f -> %.2f",
             (double)s_config.hydraulics.pressure_mv_per_bar,
             (double)in->pressure_mv_per_bar,
             (double)s_config.hydraulics.pressure_offset_mv,
             (double)in->pressure_offset_mv);
    memcpy(&s_config.hydraulics, in, sizeof(config_hydraulic_t));
    s_dirty = true;
    xSemaphoreGive(s_lock);
    return CFG_OK;
}

/* ─── Alarms ──────────────────────────────────────────────────────────── */

cfg_result_t config_get_alarms(config_alarms_t *out)
{
    if (!s_initialized || out == NULL)
    {
        return CFG_NOT_INITIALIZED;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    memcpy(out, &s_config.alarms, sizeof(config_alarms_t));
    xSemaphoreGive(s_lock);
    return CFG_OK;
}

cfg_result_t config_set_alarms(const config_alarms_t *in)
{
    if (!s_initialized || in == NULL)
    {
        return CFG_NOT_INITIALIZED;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    zic_config_t candidate = s_config;
    candidate.alarms = *in;
    if (!config_validate_safety(&candidate))
    {
        xSemaphoreGive(s_lock);
        return CFG_VALIDATION_ERROR;
    }
    ESP_LOGI(TAG, "Hydraulic alarms changed: flow %u-%u -> %u-%u x0.1 L/min, pressure %u-%u -> %u-%u mbar",
             s_config.alarms.flow_low_lpm_x10, s_config.alarms.flow_high_lpm_x10,
             in->flow_low_lpm_x10, in->flow_high_lpm_x10,
             s_config.alarms.pressure_low_mbar, s_config.alarms.pressure_high_mbar,
             in->pressure_low_mbar, in->pressure_high_mbar);
    memcpy(&s_config.alarms, in, sizeof(config_alarms_t));
    s_dirty = true;
    xSemaphoreGive(s_lock);
    return CFG_OK;
}

/* ─── Zone ────────────────────────────────────────────────────────────── */

cfg_result_t config_get_zone(uint8_t zone_index, config_zone_t *out)
{
    if (!s_initialized || out == NULL)
    {
        return CFG_NOT_INITIALIZED;
    }
    if (zone_index >= CONFIG_MAX_ZONES)
    {
        return CFG_ZONE_INVALID;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    memcpy(out, &s_config.zones[zone_index], sizeof(config_zone_t));
    xSemaphoreGive(s_lock);
    return CFG_OK;
}

cfg_result_t config_set_zone(uint8_t zone_index, const config_zone_t *in)
{
    if (!s_initialized || in == NULL)
    {
        return CFG_NOT_INITIALIZED;
    }
    if (zone_index >= CONFIG_MAX_ZONES)
    {
        return CFG_ZONE_INVALID;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    zic_config_t candidate = s_config;
    candidate.zones[zone_index] = *in;
    if (!config_validate_safety(&candidate))
    {
        xSemaphoreGive(s_lock);
        ESP_LOGW(TAG, "Zone %u: invalid safety configuration", zone_index);
        return CFG_VALIDATION_ERROR;
    }
    ESP_LOGI(TAG, "Zone %u safety changed: flow %u -> %u x0.1 L/min, pressure %u-%u -> %u-%u mbar",
             zone_index + 1,
             s_config.zones[zone_index].flow_baseline_lpm_x10,
             in->flow_baseline_lpm_x10,
             s_config.zones[zone_index].pressure_min_mbar,
             s_config.zones[zone_index].pressure_max_mbar,
             in->pressure_min_mbar, in->pressure_max_mbar);
    memcpy(&s_config.zones[zone_index], in, sizeof(config_zone_t));
    s_dirty = true;
    xSemaphoreGive(s_lock);
    return CFG_OK;
}

cfg_result_t config_commission_zone(uint8_t zone_index,
                                    uint16_t observed_flow_lpm_x10,
                                    uint16_t observed_pressure_mbar)
{
    if (!s_initialized)
    {
        return CFG_NOT_INITIALIZED;
    }
    if (zone_index >= CONFIG_MAX_ZONES)
    {
        return CFG_ZONE_INVALID;
    }

    xSemaphoreTake(s_lock, portMAX_DELAY);
    config_zone_t candidate = s_config.zones[zone_index];
    if (!config_apply_zone_commissioning(&candidate, observed_flow_lpm_x10,
                                         observed_pressure_mbar) ||
        !config_validate_zone_safety(&candidate, &s_config.system))
    {
        xSemaphoreGive(s_lock);
        return CFG_VALIDATION_ERROR;
    }
    zic_config_t candidate_config = s_config;
    candidate_config.zones[zone_index] = candidate;
    if (!config_validate_safety(&candidate_config))
    {
        xSemaphoreGive(s_lock);
        return CFG_VALIDATION_ERROR;
    }

    ESP_LOGI(TAG, "Zone %u commissioned: flow %u -> %u x0.1 L/min, pressure %u-%u -> %u-%u mbar",
             zone_index + 1,
             s_config.zones[zone_index].flow_baseline_lpm_x10,
             candidate.flow_baseline_lpm_x10,
             s_config.zones[zone_index].pressure_min_mbar,
             s_config.zones[zone_index].pressure_max_mbar,
             candidate.pressure_min_mbar, candidate.pressure_max_mbar);
    s_config.zones[zone_index] = candidate;
    s_dirty = true;
    xSemaphoreGive(s_lock);
    return CFG_OK;
}

/* ─── Program ─────────────────────────────────────────────────────────── */

cfg_result_t config_get_program(uint8_t program_index, config_program_t *out)
{
    if (!s_initialized || out == NULL)
    {
        return CFG_NOT_INITIALIZED;
    }
    if (program_index >= CONFIG_MAX_PROGRAMS)
    {
        return CFG_PROGRAM_INVALID;
    }
    xSemaphoreTake(s_lock, portMAX_DELAY);
    memcpy(out, &s_config.programs[program_index], sizeof(config_program_t));
    xSemaphoreGive(s_lock);
    return CFG_OK;
}

cfg_result_t config_set_program(uint8_t program_index, const config_program_t *in)
{
    if (!s_initialized || in == NULL)
    {
        return CFG_NOT_INITIALIZED;
    }
    if (program_index >= CONFIG_MAX_PROGRAMS)
    {
        return CFG_PROGRAM_INVALID;
    }

    xSemaphoreTake(s_lock, portMAX_DELAY);
    memcpy(&s_config.programs[program_index], in, sizeof(config_program_t));
    s_dirty = true;
    xSemaphoreGive(s_lock);
    return CFG_OK;
}

/* ─── Diagnostics ─────────────────────────────────────────────────────── */

uint16_t config_get_schema_version(void)
{
    return s_config.schema_version;
}

bool config_is_dirty(void)
{
    return s_dirty;
}
