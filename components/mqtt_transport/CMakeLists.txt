idf_component_register(
    SRCS "src/mqtt_transport.c"
    INCLUDE_DIRS "include"
    REQUIRES mqtt mqtt_manager
)
