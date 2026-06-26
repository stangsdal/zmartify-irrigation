idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES event_bus
                             zic_hal
                             config_manager
                             relay_manager
                             zone_manager
                             irrigation_engine
                             flow_manager
                             pressure_manager
                             weather_manager
                             et_engine
                             alarm_manager
                             storage_manager
                             mqtt_manager
                             ota_manager
                             diagnostics_manager)
