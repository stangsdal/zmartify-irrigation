idf_component_register(
    SRCS "src/event_bus.c"
    INCLUDE_DIRS "include"
    REQUIRES freertos esp_timer esp_common
    PRIV_INCLUDE_DIRS "private"
)
