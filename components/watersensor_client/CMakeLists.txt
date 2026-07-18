idf_component_register(
    SRCS
        "src/watersensor_client.c"
        "src/watersensor_protocol.c"
    INCLUDE_DIRS "include"
    REQUIRES
        esp_timer
        event_bus
        freertos
        zic_hal
)
