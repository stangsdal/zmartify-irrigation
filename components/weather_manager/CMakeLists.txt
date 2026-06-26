idf_component_register(
    SRCS "src/weather_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES
        freertos
        esp_timer
        esp_common
        config_manager
        event_bus
        zic_hal
)
