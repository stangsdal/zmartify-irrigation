idf_component_register(
    SRCS "src/irrigation_engine.c"
    INCLUDE_DIRS "include"
    REQUIRES
        freertos
        esp_timer
        esp_common
        zone_manager
        relay_manager
        config_manager
        event_bus
        weather_manager
)
