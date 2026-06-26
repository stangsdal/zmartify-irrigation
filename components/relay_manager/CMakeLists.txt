idf_component_register(
    SRCS "src/relay_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES
        freertos
        esp_common
        zic_hal
        event_bus
)
