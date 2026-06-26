idf_component_register(
    SRCS
        "src/hal.c"
        "src/hal_gpio.c"
        "src/hal_i2c.c"
        "src/hal_relay.c"
        "src/hal_flow.c"
        "src/hal_pressure.c"
        "src/hal_time.c"
        "src/hal_storage.c"
    INCLUDE_DIRS "include"
    PRIV_INCLUDE_DIRS "private"
    REQUIRES
        freertos
        esp_timer
        esp_common
        driver
        esp_driver_gpio
        esp_driver_i2c
        esp_driver_pcnt
        nvs_flash
        lwip
        esp_netif
)
