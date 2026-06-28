file(GLOB_RECURSE srcs *.c
                   ui/*.c
                   ui/components/*.c
                   ui/screens/*.c
                   user/add_password/*.c
                   user/wifi/*.c
                   user/rs485/*.c
                   user/can/*.c
                   user/pwm/*.c
                   user/sd_card/*.c
                   )
set(include_dirs 
   . 
   ui
   user/add_password
   user/wifi
   user/rs485
   user/can
   user/pwm
   user/sd_card
   )
idf_component_register( SRCS ${srcs}
                       INCLUDE_DIRS ${include_dirs}
                       REQUIRES esp_event esp_netif esp_wifi mbedtls nvs_flash
                       PRIV_REQUIRES fonts gpio gui_paint i2c image io_extension lvgl_port rgb_lcd_port sd touch twai usart
                       )
idf_component_get_property(lvgl_lib lvgl__lvgl COMPONENT_LIB)
target_compile_options(${lvgl_lib} PRIVATE -Wno-format)