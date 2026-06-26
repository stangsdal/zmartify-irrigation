idf_component_register(
    SRCS "src/diagnostics_manager.c"
    INCLUDE_DIRS "include"
    REQUIRES
        freertos
        esp_timer
        esp_common
        esp_system
        heap
        app_update
        event_bus
        alarm_manager
        storage_manager
)
