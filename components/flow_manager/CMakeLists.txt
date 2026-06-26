idf_component_register(
    SRCS "src/flow_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES
        freertos
        esp_timer
        esp_common
        zic_hal
        config_manager
        event_bus
)
