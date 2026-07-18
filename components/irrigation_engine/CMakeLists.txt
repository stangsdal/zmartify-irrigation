idf_component_register(
    SRCS "src/irrigation_engine.c"
    INCLUDE_DIRS "include"
    REQUIRES relay_manager zic_core zone_manager
)
