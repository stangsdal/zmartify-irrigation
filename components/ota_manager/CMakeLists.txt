idf_component_register(
    SRCS "src/ota_manager.c" "src/ota_policy.c"
    INCLUDE_DIRS "include"
    REQUIRES app_update cjson esp_https_ota esp_http_client mbedtls nvs_flash freertos
)
