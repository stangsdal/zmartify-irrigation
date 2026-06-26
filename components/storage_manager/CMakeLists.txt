idf_component_register(
    SRCS "src/storage_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES
        freertos
        esp_timer
        esp_common
        zic_hal
        event_bus
)
