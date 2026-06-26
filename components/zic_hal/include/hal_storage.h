/**
 * @file hal_storage.h
 * @brief Storage HAL - NVS (Non-Volatile Storage) wrapper
 *
 * Provides typed key-value storage backed by ESP-IDF NVS.
 * Callers never interact with NVS handles directly.
 *
 * Architecture ref: MEP v5.0 Volume 2, Chapter 4, Section 4.14
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>

typedef enum hal_result hal_result_t;

/** NVS namespace used by the application */
#define HAL_STORAGE_NAMESPACE  "zic_config"

/**
 * @brief Initialise NVS flash.
 *
 * Must be called before any read/write operations.
 * Erases and re-initialises NVS if the partition is found to be corrupt.
 *
 * @return HAL_OK on success.
 */
hal_result_t hal_storage_init(void);

/**
 * @brief Write a 32-bit unsigned integer.
 */
hal_result_t hal_storage_write_u32(const char *key, uint32_t value);

/**
 * @brief Read a 32-bit unsigned integer.
 *
 * @param key           NVS key
 * @param value         Output value
 * @param default_val   Value to return when key does not exist
 */
hal_result_t hal_storage_read_u32(const char *key, uint32_t *value,
                                   uint32_t default_val);

/**
 * @brief Write a 32-bit signed integer.
 */
hal_result_t hal_storage_write_i32(const char *key, int32_t value);

/**
 * @brief Read a 32-bit signed integer.
 */
hal_result_t hal_storage_read_i32(const char *key, int32_t *value,
                                   int32_t default_val);

/**
 * @brief Write a boolean value (stored as uint8_t).
 */
hal_result_t hal_storage_write_bool(const char *key, bool value);

/**
 * @brief Read a boolean value.
 */
hal_result_t hal_storage_read_bool(const char *key, bool *value,
                                    bool default_val);

/**
 * @brief Write a null-terminated string (max HAL_STORAGE_STR_MAX bytes).
 */
#define HAL_STORAGE_STR_MAX  128
hal_result_t hal_storage_write_str(const char *key, const char *str);

/**
 * @brief Read a null-terminated string.
 *
 * @param key     NVS key
 * @param buf     Destination buffer
 * @param len     Buffer size (must be > 0)
 * @param default_str  Default string if key absent; may be NULL.
 */
hal_result_t hal_storage_read_str(const char *key, char *buf, size_t len,
                                   const char *default_str);

/**
 * @brief Write a raw binary blob (max 4096 bytes).
 */
hal_result_t hal_storage_write_blob(const char *key,
                                     const void *data, size_t len);

/**
 * @brief Read a raw binary blob.
 *
 * @param key      NVS key
 * @param buf      Destination buffer
 * @param len      In: buffer size; Out: bytes read
 */
hal_result_t hal_storage_read_blob(const char *key, void *buf, size_t *len);

/**
 * @brief Delete a key from NVS.
 */
hal_result_t hal_storage_erase_key(const char *key);

/**
 * @brief Erase all keys in the application namespace.
 *
 * Use with caution – factory reset operation.
 */
hal_result_t hal_storage_erase_all(void);
