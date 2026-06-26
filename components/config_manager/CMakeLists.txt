idf_component_register(
    SRCS "src/config_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES
        freertos
        esp_common
        nvs_flash
        esp_timer
        zic_hal
        event_bus
)
