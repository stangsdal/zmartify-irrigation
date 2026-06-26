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
} cfg_result_t;

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
