/*
 * hal_storage.c
 * Storage HAL – NVS wrapper
 */

#include "hal.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

static const char *TAG = "hal_storage";
static bool s_initialized = false;

hal_result_t hal_storage_init(void)
{
    if (s_initialized)
    {
        return HAL_OK;
    }

    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_LOGW(TAG, "NVS partition corrupt – erasing and re-initialising");
        nvs_flash_erase();
        err = nvs_flash_init();
    }

    if (err != ESP_OK)
    {
        ESP_LOGE(TAG, "nvs_flash_init failed: %d", err);
        return HAL_IO_ERROR;
    }

    s_initialized = true;
    ESP_LOGI(TAG, "Storage HAL initialized (namespace: %s)", HAL_STORAGE_NAMESPACE);
    return HAL_OK;
}

/** Open NVS handle – caller must call nvs_close() */
static esp_err_t open_nvs(nvs_open_mode_t mode, nvs_handle_t *handle)
{
    return nvs_open(HAL_STORAGE_NAMESPACE, mode, handle);
}

hal_result_t hal_storage_write_u32(const char *key, uint32_t value)
{
    if (!s_initialized || key == NULL)
    {
        return HAL_NOT_INITIALIZED;
    }
    nvs_handle_t h;
    if (open_nvs(NVS_READWRITE, &h) != ESP_OK)
    {
        return HAL_IO_ERROR;
    }
    esp_err_t err = nvs_set_u32(h, key, value);
    if (err == ESP_OK)
    {
        err = nvs_commit(h);
    }
    nvs_close(h);
    return (err == ESP_OK) ? HAL_OK : HAL_IO_ERROR;
}

hal_result_t hal_storage_read_u32(const char *key, uint32_t *value,
                                    uint32_t default_val)
{
    if (!s_initialized || key == NULL || value == NULL)
    {
        return HAL_NOT_INITIALIZED;
    }
    nvs_handle_t h;
    if (open_nvs(NVS_READONLY, &h) != ESP_OK)
    {
        *value = default_val;
        return HAL_OK;
    }
    esp_err_t err = nvs_get_u32(h, key, value);
    nvs_close(h);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        *value = default_val;
        return HAL_OK;
    }
    return (err == ESP_OK) ? HAL_OK : HAL_IO_ERROR;
}

hal_result_t hal_storage_write_i32(const char *key, int32_t value)
{
    if (!s_initialized || key == NULL)
    {
        return HAL_NOT_INITIALIZED;
    }
    nvs_handle_t h;
    if (open_nvs(NVS_READWRITE, &h) != ESP_OK)
    {
        return HAL_IO_ERROR;
    }
    esp_err_t err = nvs_set_i32(h, key, value);
    if (err == ESP_OK)
    {
        err = nvs_commit(h);
    }
    nvs_close(h);
    return (err == ESP_OK) ? HAL_OK : HAL_IO_ERROR;
}

hal_result_t hal_storage_read_i32(const char *key, int32_t *value,
                                    int32_t default_val)
{
    if (!s_initialized || key == NULL || value == NULL)
    {
        return HAL_NOT_INITIALIZED;
    }
    nvs_handle_t h;
    if (open_nvs(NVS_READONLY, &h) != ESP_OK)
    {
        *value = default_val;
        return HAL_OK;
    }
    esp_err_t err = nvs_get_i32(h, key, value);
    nvs_close(h);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        *value = default_val;
        return HAL_OK;
    }
    return (err == ESP_OK) ? HAL_OK : HAL_IO_ERROR;
}

hal_result_t hal_storage_write_bool(const char *key, bool value)
{
    return hal_storage_write_u32(key, value ? 1u : 0u);
}

hal_result_t hal_storage_read_bool(const char *key, bool *value,
                                     bool default_val)
{
    if (value == NULL)
    {
        return HAL_INVALID_PARAMETER;
    }
    uint32_t v = 0;
    hal_result_t r = hal_storage_read_u32(key, &v, default_val ? 1u : 0u);
    *value = (v != 0);
    return r;
}

hal_result_t hal_storage_write_str(const char *key, const char *str)
{
    if (!s_initialized || key == NULL || str == NULL)
    {
        return HAL_NOT_INITIALIZED;
    }
    nvs_handle_t h;
    if (open_nvs(NVS_READWRITE, &h) != ESP_OK)
    {
        return HAL_IO_ERROR;
    }
    esp_err_t err = nvs_set_str(h, key, str);
    if (err == ESP_OK)
    {
        err = nvs_commit(h);
    }
    nvs_close(h);
    return (err == ESP_OK) ? HAL_OK : HAL_IO_ERROR;
}

hal_result_t hal_storage_read_str(const char *key, char *buf, size_t len,
                                    const char *default_str)
{
    if (!s_initialized || key == NULL || buf == NULL || len == 0)
    {
        return HAL_NOT_INITIALIZED;
    }
    nvs_handle_t h;
    if (open_nvs(NVS_READONLY, &h) != ESP_OK)
    {
        if (default_str != NULL)
        {
            strncpy(buf, default_str, len - 1);
            buf[len - 1] = '\0';
        }
        else
        {
            buf[0] = '\0';
        }
        return HAL_OK;
    }
    esp_err_t err = nvs_get_str(h, key, buf, &len);
    nvs_close(h);
    if (err == ESP_ERR_NVS_NOT_FOUND)
    {
        if (default_str != NULL)
        {
            strncpy(buf, default_str, len - 1);
            buf[len - 1] = '\0';
        }
        else
        {
            buf[0] = '\0';
        }
        return HAL_OK;
    }
    return (err == ESP_OK) ? HAL_OK : HAL_IO_ERROR;
}

hal_result_t hal_storage_write_blob(const char *key,
                                      const void *data, size_t len)
{
    if (!s_initialized || key == NULL || data == NULL || len == 0)
    {
        return HAL_NOT_INITIALIZED;
    }
    nvs_handle_t h;
    if (open_nvs(NVS_READWRITE, &h) != ESP_OK)
    {
        return HAL_IO_ERROR;
    }
    esp_err_t err = nvs_set_blob(h, key, data, len);
    if (err == ESP_OK)
    {
        err = nvs_commit(h);
    }
    nvs_close(h);
    return (err == ESP_OK) ? HAL_OK : HAL_IO_ERROR;
}

hal_result_t hal_storage_read_blob(const char *key, void *buf, size_t *len)
{
    if (!s_initialized || key == NULL || buf == NULL || len == NULL)
    {
        return HAL_NOT_INITIALIZED;
    }
    nvs_handle_t h;
    if (open_nvs(NVS_READONLY, &h) != ESP_OK)
    {
        return HAL_IO_ERROR;
    }
    esp_err_t err = nvs_get_blob(h, key, buf, len);
    nvs_close(h);
    return (err == ESP_OK) ? HAL_OK : HAL_IO_ERROR;
}

hal_result_t hal_storage_erase_key(const char *key)
{
    if (!s_initialized || key == NULL)
    {
        return HAL_NOT_INITIALIZED;
    }
    nvs_handle_t h;
    if (open_nvs(NVS_READWRITE, &h) != ESP_OK)
    {
        return HAL_IO_ERROR;
    }
    esp_err_t err = nvs_erase_key(h, key);
    if (err == ESP_OK)
    {
        nvs_commit(h);
    }
    nvs_close(h);
    return (err == ESP_OK || err == ESP_ERR_NVS_NOT_FOUND) ? HAL_OK : HAL_IO_ERROR;
}

hal_result_t hal_storage_erase_all(void)
{
    if (!s_initialized)
    {
        return HAL_NOT_INITIALIZED;
    }
    nvs_handle_t h;
    if (open_nvs(NVS_READWRITE, &h) != ESP_OK)
    {
        return HAL_IO_ERROR;
    }
    esp_err_t err = nvs_erase_all(h);
    if (err == ESP_OK)
    {
        nvs_commit(h);
    }
    nvs_close(h);
    ESP_LOGW(TAG, "NVS namespace '%s' erased", HAL_STORAGE_NAMESPACE);
    return (err == ESP_OK) ? HAL_OK : HAL_IO_ERROR;
}
