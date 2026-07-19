idf_component_register(
    SRCS "src/ota_manager.c" "src/ota_policy.c"
    INCLUDE_DIRS "include"
    REQUIRES esp_https_ota app_update mbedtls freertos
)
