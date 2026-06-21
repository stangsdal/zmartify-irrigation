idf_component_register(
    SRCS "src/persistent_store.c"
    INCLUDE_DIRS "include"
    REQUIRES nvs_flash
)
