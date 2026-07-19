idf_component_register(
    SRCS "src/mqtt_transport.c"
    INCLUDE_DIRS "include"
    REQUIRES esp-tls mqtt mqtt_manager
)
