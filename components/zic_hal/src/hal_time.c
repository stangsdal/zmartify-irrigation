/*
 * hal_time.c
 * Time HAL – SNTP client + POSIX time wrapper
 */

#include "hal.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_sntp.h"
#include <string.h>
#include <time.h>
#include <sys/time.h>

static const char *TAG = "hal_time";

static bool s_initialized = false;
static bool s_synced      = false;

static void sntp_sync_cb(struct timeval *tv)
{
    (void)tv;
    s_synced = true;
    ESP_LOGI(TAG, "SNTP time synchronized");
}

hal_result_t hal_time_init(const char *tz_posix)
{
    if (s_initialized)
    {
        return HAL_OK;
    }

    /* Set timezone before sync so localtime() is correct on first use */
    if (tz_posix != NULL && tz_posix[0] != '\0')
    {
        setenv("TZ", tz_posix, 1);
        tzset();
        ESP_LOGI(TAG, "Timezone set: %s", tz_posix);
    }
    else
    {
        setenv("TZ", "UTC0", 1);
        tzset();
    }

    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, HAL_TIME_NTP_SERVER);
    esp_sntp_set_time_sync_notification_cb(sntp_sync_cb);
    esp_sntp_init();

    s_initialized = true;
    ESP_LOGI(TAG, "Time HAL initialized (NTP: %s)", HAL_TIME_NTP_SERVER);
    return HAL_OK;
}

bool hal_time_is_synced(void)
{
    return s_synced;
}

hal_result_t hal_time_get_epoch(uint32_t *epoch)
{
    if (!s_initialized || epoch == NULL)
    {
        return HAL_NOT_INITIALIZED;
    }
    *epoch = (uint32_t)time(NULL);
    return HAL_OK;
}

hal_result_t hal_time_get_local(struct tm *tm_out)
{
    if (!s_initialized || tm_out == NULL)
    {
        return HAL_NOT_INITIALIZED;
    }
    time_t now = time(NULL);
    localtime_r(&now, tm_out);
    return HAL_OK;
}

hal_result_t hal_time_get_uptime_ms(uint64_t *uptime_ms)
{
    if (uptime_ms == NULL)
    {
        return HAL_INVALID_PARAMETER;
    }
    *uptime_ms = (uint64_t)(esp_timer_get_time() / 1000);
    return HAL_OK;
}

hal_result_t hal_time_sync_now(void)
{
    if (!s_initialized)
    {
        return HAL_NOT_INITIALIZED;
    }
    s_synced = false;
    esp_sntp_restart();
    ESP_LOGI(TAG, "SNTP resync requested");
    return HAL_OK;
}
