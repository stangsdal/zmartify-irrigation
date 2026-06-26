idf_component_register(
    SRCS "src/mqtt_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES
        freertos
        esp_wifi
        esp_event
        esp_netif
        esp_common
        esp_timer
        espressif__mqtt
        config_manager
        event_bus
        irrigation_engine
        zone_manager
        weather_manager
        flow_manager
        pressure_manager
        alarm_manager
        storage_manager
)
