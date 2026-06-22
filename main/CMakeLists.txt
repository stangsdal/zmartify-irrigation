idf_component_register(SRCS "main.c"
                    INCLUDE_DIRS "."
                    REQUIRES alarm_manager
                             et_engine
                             flow_manager
                             irrigation_engine
                             mqtt_manager
                             mqtt_transport
                             ota_manager
                             persistent_store
                             pressure_manager
                             storage_manager
                             weather_manager
                             cjson)
