/**
 * @file config_types.h
 * @brief Zmartify ZIC-S3 – Configuration data types
 *
 * Defines the complete persistent configuration schema for the controller.
 * Every configurable parameter is declared here.
 *
 * Schema version history:
 *   1  – v5.0 initial schema
 *
 * Architecture ref: MEP v5.0 Volume 2, Chapter 14
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>

/* ─── Schema & limits ────────────────────────────────────────────────── */

#define CONFIG_SCHEMA_VERSION    1       /**< Increment on incompatible changes */
#define CONFIG_MAGIC             0x5A49  /**< 'ZI' – used as header sanity check */

#define CONFIG_MAX_ZONES         15      /**< Relays 1-15 are irrigation zones */
#define CONFIG_MAX_PROGRAMS      8       /**< Irrigation programs */
#define CONFIG_MAX_START_TIMES   4       /**< Start times per program per day */
#define CONFIG_SSID_LEN          64
#define CONFIG_PASSWORD_LEN      64
#define CONFIG_MQTT_URI_LEN      128
#define CONFIG_MQTT_USER_LEN     48
#define CONFIG_MQTT_PASS_LEN     48
#define CONFIG_ZONE_NAME_LEN     32
#define CONFIG_PROGRAM_NAME_LEN  32
#define CONFIG_TZ_LEN            64      /**< POSIX TZ string */
#define CONFIG_NTP_LEN           64

/* ─── Enumerations ───────────────────────────────────────────────────── */

/**
 * @brief Controller operational mode
 */
typedef enum
{
    CONFIG_MODE_AUTO     = 0,  /**< Normal automatic scheduling */
    CONFIG_MODE_MANUAL   = 1,  /**< Manual zone control only */
    CONFIG_MODE_OFF      = 2,  /**< All irrigation disabled */
    CONFIG_MODE_SERVICE  = 3,  /**< Service / diagnostics mode */
} config_op_mode_t;

/**
 * @brief Plant type – influences ET calculation coefficients
 */
typedef enum
{
    CONFIG_PLANT_LAWN       = 0,
    CONFIG_PLANT_SHRUBS     = 1,
    CONFIG_PLANT_TREES      = 2,
    CONFIG_PLANT_VEGETABLES = 3,
    CONFIG_PLANT_FLOWERS    = 4,
    CONFIG_PLANT_OTHER      = 5,
} config_plant_type_t;

/**
 * @brief Soil type – influences infiltration rate
 */
typedef enum
{
    CONFIG_SOIL_SANDY  = 0,
    CONFIG_SOIL_LOAM   = 1,
    CONFIG_SOIL_CLAY   = 2,
    CONFIG_SOIL_OTHER  = 3,
} config_soil_type_t;

/**
 * @brief Sprinkler / emitter type – influences precipitation rate
 */
typedef enum
{
    CONFIG_EMITTER_ROTOR    = 0,
    CONFIG_EMITTER_SPRAY    = 1,
    CONFIG_EMITTER_DRIP     = 2,
    CONFIG_EMITTER_BUBBLER  = 3,
    CONFIG_EMITTER_OTHER    = 4,
} config_emitter_type_t;

/**
 * @brief Schedule days bitmask (bit 0 = Sunday ... bit 6 = Saturday)
 */
typedef uint8_t config_days_t;
#define CONFIG_DAY_SUN  (1u << 0)
#define CONFIG_DAY_MON  (1u << 1)
#define CONFIG_DAY_TUE  (1u << 2)
#define CONFIG_DAY_WED  (1u << 3)
#define CONFIG_DAY_THU  (1u << 4)
#define CONFIG_DAY_FRI  (1u << 5)
#define CONFIG_DAY_SAT  (1u << 6)
#define CONFIG_DAYS_ALL (0x7Fu)

/* ─── Zone configuration ──────────────────────────────────────────────── */

/**
 * @brief Per-zone configuration
 */
typedef struct
{
    char                 name[CONFIG_ZONE_NAME_LEN];  /**< Display name */
    uint8_t              relay_index;                  /**< MCP23017 relay (1–15) */
    bool                 enabled;
    config_plant_type_t  plant_type;
    config_soil_type_t   soil_type;
    config_emitter_type_t emitter_type;
    uint16_t             area_m2;                      /**< Irrigated area in m² */
    uint32_t             default_runtime_s;            /**< Default run time (seconds) */
    uint32_t             max_runtime_s;                /**< Safety maximum (seconds) */
    uint16_t             flow_baseline_lpm_x10;        /**< Expected L/min × 10 */
    uint16_t             pressure_min_mbar;            /**< Min acceptable pressure */
    uint16_t             pressure_max_mbar;            /**< Max acceptable pressure */
    uint8_t              seasonal_factor_pct;          /**< 0–200 %, 100 = nominal */
    uint8_t              et_crop_coefficient_x100;     /**< Kc × 100, e.g. 80 = 0.80 */
    uint8_t              _reserved[6];
} config_zone_t;

/* ─── Program start time ──────────────────────────────────────────────── */

/**
 * @brief A single start time entry
 */
typedef struct
{
    bool    enabled;
    uint8_t hour;    /**< 0-23 */
    uint8_t minute;  /**< 0-59 */
} config_start_time_t;

/* ─── Program configuration ───────────────────────────────────────────── */

/**
 * @brief Irrigation program (schedule of zone runtimes)
 */
typedef struct
{
    char                  name[CONFIG_PROGRAM_NAME_LEN];
    bool                  enabled;
    config_days_t         run_days;                           /**< Bitmask of active days */
    config_start_time_t   start_times[CONFIG_MAX_START_TIMES];
    uint8_t               zone_runtime_min[CONFIG_MAX_ZONES]; /**< Minutes per zone (0=skip) */
    bool                  weather_skip_enabled;               /**< Skip on rain forecast */
    uint8_t               rain_skip_threshold_pct;            /**< Rain probability % */
    uint8_t               seasonal_adjust_pct;               /**< Global seasonal factor */
    uint8_t               _reserved[4];
} config_program_t;

/* ─── System configuration ────────────────────────────────────────────── */

/**
 * @brief System-level controller settings
 */
typedef struct
{
    uint32_t         controller_id;         /**< Unique controller identifier */
    config_op_mode_t operational_mode;
    uint8_t          active_zone_count;     /**< Number of zones in use */
    uint32_t         max_simultaneous_zones;/**< Max concurrent open valves */
    uint32_t         global_max_runtime_s;  /**< Hard safety limit */
    bool             master_valve_enabled;
    uint8_t          _reserved[7];
} config_system_t;

/* ─── Network configuration ───────────────────────────────────────────── */

/**
 * @brief Wi-Fi and MQTT connection settings
 */
typedef struct
{
    char     ssid[CONFIG_SSID_LEN];
    char     password[CONFIG_PASSWORD_LEN];
    char     mqtt_broker_uri[CONFIG_MQTT_URI_LEN];
    char     mqtt_username[CONFIG_MQTT_USER_LEN];
    char     mqtt_password[CONFIG_MQTT_PASS_LEN];
    char     ntp_server[CONFIG_NTP_LEN];
    char     timezone[CONFIG_TZ_LEN];          /**< POSIX TZ string */
    bool     mqtt_tls_enabled;
    uint16_t mqtt_port;                        /**< 0 = use URI default */
    uint8_t  wifi_max_retries;
    uint8_t  _reserved[5];
} config_network_t;

/* ─── Hydraulic calibration ────────────────────────────────────────────── */

/**
 * @brief Flow & pressure sensor calibration coefficients
 */
typedef struct
{
    float    flow_pulses_per_litre;   /**< Sensor K-factor (pulses/L) */
    float    pressure_mv_per_bar;     /**< mV/bar from pressure transmitter */
    float    pressure_offset_mv;      /**< Zero-offset in millivolts */
    float    pressure_min_bar;        /**< Operational minimum (for alarm) */
    float    pressure_max_bar;        /**< Operational maximum (for alarm) */
    uint8_t  _reserved[8];
} config_hydraulic_t;

/* ─── Alarm thresholds ─────────────────────────────────────────────────── */

/**
 * @brief Alarm and safety limit settings
 */
typedef struct
{
    uint16_t pressure_low_mbar;          /**< Low pressure alarm */
    uint16_t pressure_high_mbar;         /**< High pressure alarm */
    uint16_t flow_high_lpm_x10;          /**< High flow (leak) alarm, L/min × 10 */
    uint16_t flow_low_lpm_x10;           /**< Low flow (valve fault) alarm */
    uint32_t no_flow_timeout_s;          /**< No-flow timeout when valve open */
    uint32_t high_flow_duration_s;       /**< Duration before high-flow triggers alarm */
    uint8_t  cabinet_warn_temp_c;        /**< Cabinet temperature warning */
    uint8_t  cabinet_crit_temp_c;        /**< Cabinet temperature critical */
    uint8_t  _reserved[6];
} config_alarms_t;

/* ─── Top-level configuration database ────────────────────────────────── */

/**
 * @brief Complete configuration database
 *
 * Stored as a single NVS blob with CRC32 integrity check.
 * Any schema change increments CONFIG_SCHEMA_VERSION.
 */
typedef struct
{
    /* Header */
    uint16_t magic;           /**< CONFIG_MAGIC */
    uint16_t schema_version;  /**< CONFIG_SCHEMA_VERSION */
    uint32_t crc32;           /**< CRC32 of all bytes after this field */

    /* Subsystems */
    config_system_t    system;
    config_network_t   network;
    config_hydraulic_t hydraulics;
    config_alarms_t    alarms;
    config_zone_t      zones[CONFIG_MAX_ZONES];
    config_program_t   programs[CONFIG_MAX_PROGRAMS];
} zic_config_t;
