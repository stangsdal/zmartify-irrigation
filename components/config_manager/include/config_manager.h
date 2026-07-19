/**
 * @file config_manager.h
 * @brief Configuration Manager – centralised config API
 *
 * The Configuration Manager is the only component permitted to read
 * or write persistent configuration. All other modules call this API.
 *
 * Features:
 *  - Full typed configuration schema (config_types.h)
 *  - CRC32 integrity protection on NVS blob
 *  - Schema versioning with automatic migration
 *  - Factory defaults with no erasure of lifetime stats
 *  - Thread-safe (mutex-protected) read/write
 *  - Event Bus publication on every commit
 *
 * Architecture ref: MEP v5.0 Volume 2, Chapter 14
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include "config_types.h"

/* ─── Result type ────────────────────────────────────────────────────── */

typedef enum
{
    CFG_OK              = 0,
    CFG_NOT_INITIALIZED = 1,
    CFG_INVALID_PARAM   = 2,
    CFG_STORAGE_ERROR   = 3,
    CFG_CRC_ERROR       = 4,
    CFG_VERSION_MISMATCH= 5,
    CFG_ZONE_INVALID    = 6,
    CFG_PROGRAM_INVALID = 7,
    CFG_VALIDATION_ERROR= 8,
    CFG_SAFE_MODE       = 9,
} cfg_result_t;

typedef enum
{
    CONFIG_CHANGE_NONE       = 0,
    CONFIG_CHANGE_SYSTEM     = 1u << 0,
    CONFIG_CHANGE_NETWORK    = 1u << 1,
    CONFIG_CHANGE_HYDRAULICS = 1u << 2,
    CONFIG_CHANGE_ALARMS     = 1u << 3,
    CONFIG_CHANGE_ZONES      = 1u << 4,
    CONFIG_CHANGE_PROGRAMS   = 1u << 5,
    CONFIG_CHANGE_ALL        = 0x3Fu,
} config_change_mask_t;

typedef struct
{
    uint8_t payload_version;
    uint8_t payload_size;
    uint16_t schema_version;
    uint32_t changed_sections;
    uint8_t factory_reset;
    uint8_t reserved[3];
} config_change_event_t;

#define CONFIG_CHANGE_EVENT_VERSION 1u

/* ─── Lifecycle ───────────────────────────────────────────────────────── */

/**
 * @brief Initialise the Configuration Manager.
 *
 * Loads configuration from NVS. On corruption or first boot,
 * factory defaults are written automatically.
 *
 * @return CFG_OK on success.
 */
cfg_result_t config_manager_init(void);

/**
 * @brief Commit pending in-memory changes to NVS.
 *
 * Updates CRC32, writes the blob, publishes EVENT_CONFIG_CHANGED.
 * @return CFG_OK on success.
 */
cfg_result_t config_manager_commit(void);

/**
 * @brief Restore all settings to factory defaults and commit.
 *
 * @return CFG_OK on success.
 */
cfg_result_t config_manager_restore_defaults(void);

/* ─── Bulk access ─────────────────────────────────────────────────────── */

/**
 * @brief Get a read-only pointer to the full configuration.
 *
 * The returned pointer is valid until the next commit or restore.
 * Callers must not modify the struct via this pointer.
 */
const zic_config_t *config_get(void);

/**
 * @brief Copy the full configuration into a caller-supplied buffer.
 */
cfg_result_t config_copy(zic_config_t *dst);

/* ─── System ──────────────────────────────────────────────────────────── */

cfg_result_t config_get_system(config_system_t *out);
cfg_result_t config_set_system(const config_system_t *in);

/* ─── Network ─────────────────────────────────────────────────────────── */

cfg_result_t config_get_network(config_network_t *out);
cfg_result_t config_set_network(const config_network_t *in);

/* ─── Hydraulics ──────────────────────────────────────────────────────── */

cfg_result_t config_get_hydraulics(config_hydraulic_t *out);
cfg_result_t config_set_hydraulics(const config_hydraulic_t *in);

/**
 * @brief Apply observed stable flow and pressure as a zone commissioning baseline.
 *
 * Sets expected flow and a conservative +/-20 percent pressure envelope. The
 * change remains in RAM until config_manager_commit() succeeds.
 */
cfg_result_t config_commission_zone(uint8_t zone_index,
                                    uint16_t observed_flow_lpm_x10,
                                    uint16_t observed_pressure_mbar);

/* ─── Alarms ──────────────────────────────────────────────────────────── */

cfg_result_t config_get_alarms(config_alarms_t *out);
cfg_result_t config_set_alarms(const config_alarms_t *in);

/* ─── Zone ────────────────────────────────────────────────────────────── */

/**
 * @brief Read configuration for a single zone.
 * @param zone_index  0-based index (0 = Zone 1)
 */
cfg_result_t config_get_zone(uint8_t zone_index, config_zone_t *out);

/**
 * @brief Write configuration for a single zone.
 * Changes are held in RAM until config_manager_commit() is called.
 */
cfg_result_t config_set_zone(uint8_t zone_index, const config_zone_t *in);

/* ─── Program ─────────────────────────────────────────────────────────── */

/**
 * @brief Read configuration for a single irrigation program.
 * @param program_index  0-based index
 */
cfg_result_t config_get_program(uint8_t program_index, config_program_t *out);

/**
 * @brief Write configuration for a single program.
 */
cfg_result_t config_set_program(uint8_t program_index, const config_program_t *in);

/* ─── Diagnostics ─────────────────────────────────────────────────────── */

/**
 * @brief Schema version stored in NVS (useful for diagnostics).
 */
uint16_t config_get_schema_version(void);

/**
 * @brief Check whether configuration has been modified but not committed.
 */
bool config_is_dirty(void);

/**
 * @brief True when boot recovered from an invalid stored configuration.
 *
 * Safe mode uses validated defaults with irrigation disabled and rejects normal commits.
 */
bool config_is_safe_mode(void);

/** @brief True when the original failed or pre-migration blob is retained in NVS. */
bool config_has_recovery_snapshot(void);
