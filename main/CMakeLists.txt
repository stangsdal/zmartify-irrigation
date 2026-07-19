idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES cjson
                             alarm_manager
                             et_engine
                             esp_event
                             esp_http_server
                             esp_netif
                             app_update
                             esp_wifi
                             config_manager
                             diagnostics_manager
                             event_bus
                             flow_manager
                             hmi_board
                             irrigation_engine
                             mqtt_manager
                             mqtt_transport
                             nvs_flash
                             ota_manager
                             persistent_store
                             pressure_manager
                             relay_manager
                             storage_manager
                             weather_manager
                             watersensor_client
                             zic_hal
                             zic_v2)

if(ZIC_RELAY_SELF_TEST)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ZIC_RELAY_SELF_TEST=1)
endif()

if(ZIC_OTA_FORCE_HEALTH_FAILURE)
    target_compile_definitions(${COMPONENT_LIB} PRIVATE ZIC_OTA_FORCE_HEALTH_FAILURE=1)
endif()
