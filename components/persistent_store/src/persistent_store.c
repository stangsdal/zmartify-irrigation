#include "persistent_store.h"

#include "esp_err.h"
#include "nvs.h"
#include "nvs_flash.h"

#define ZIC_NVS_NAMESPACE "zic_cfg"

bool persistent_store_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        if (nvs_flash_erase() != ESP_OK) {
            return false;
        }
        err = nvs_flash_init();
    }

    return err == ESP_OK;
}

bool persistent_store_set_u32(const char *key, uint32_t value)
{
    nvs_handle_t handle;
    if (nvs_open(ZIC_NVS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) {
        return false;
    }

    esp_err_t err = nvs_set_u32(handle, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err == ESP_OK;
}

bool persistent_store_get_u32(const char *key, uint32_t *value_out, uint32_t default_value)
{
    nvs_handle_t handle;
    if (value_out == 0) {
        return false;
    }

    if (nvs_open(ZIC_NVS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) {
        *value_out = default_value;
        return false;
    }

    esp_err_t err = nvs_get_u32(handle, key, value_out);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *value_out = default_value;
        err = ESP_OK;
    }

    nvs_close(handle);
    return err == ESP_OK;
}

bool persistent_store_set_i32(const char *key, int32_t value)
{
    nvs_handle_t handle;
    if (nvs_open(ZIC_NVS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) {
        return false;
    }

    esp_err_t err = nvs_set_i32(handle, key, value);
    if (err == ESP_OK) {
        err = nvs_commit(handle);
    }
    nvs_close(handle);
    return err == ESP_OK;
}

bool persistent_store_get_i32(const char *key, int32_t *value_out, int32_t default_value)
{
    nvs_handle_t handle;
    if (value_out == 0) {
        return false;
    }

    if (nvs_open(ZIC_NVS_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) {
        *value_out = default_value;
        return false;
    }

    esp_err_t err = nvs_get_i32(handle, key, value_out);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        *value_out = default_value;
        err = ESP_OK;
    }

    nvs_close(handle);
    return err == ESP_OK;
}
