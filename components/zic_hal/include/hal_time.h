/**
 * @file hal_time.h
 * @brief Time HAL - RTC / NTP timekeeping abstraction
 *
 * Wraps SNTP + POSIX time for the application.
 * All timestamps in the firmware must come from this module.
 *
 * Architecture ref: MEP v5.0 Volume 2, Chapter 4, Section 4.15
 */

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

typedef enum hal_result hal_result_t;

/** Default NTP server */
#define HAL_TIME_NTP_SERVER  "pool.ntp.org"

/**
 * @brief Initialise the time subsystem.
 *
 * Configures the SNTP client. Synchronisation is asynchronous;
 * use hal_time_is_synced() to check status.
 *
 * @param tz_posix  POSIX TZ string, e.g. "AEST-10AEDT,M10.1.0,M4.1.0/3"
 *                  Pass NULL to use UTC.
 * @return HAL_OK on success.
 */
hal_result_t hal_time_init(const char *tz_posix);

/**
 * @brief Check whether NTP synchronisation has completed.
 */
bool hal_time_is_synced(void);

/**
 * @brief Get the current UTC epoch time in seconds.
 *
 * @param epoch  Output: seconds since 1970-01-01 00:00:00 UTC
 * @return HAL_OK; HAL_NOT_INITIALIZED if NTP not yet synced.
 */
hal_result_t hal_time_get_epoch(uint32_t *epoch);

/**
 * @brief Get a broken-down local time struct.
 *
 * @param tm_out  Output: filled struct tm in local time zone
 * @return HAL_OK on success.
 */
hal_result_t hal_time_get_local(struct tm *tm_out);

/**
 * @brief Get system uptime in milliseconds (always available, no NTP needed).
 *
 * @param uptime_ms  Output: milliseconds since boot
 */
hal_result_t hal_time_get_uptime_ms(uint64_t *uptime_ms);

/**
 * @brief Force an immediate NTP resync request.
 */
hal_result_t hal_time_sync_now(void);
