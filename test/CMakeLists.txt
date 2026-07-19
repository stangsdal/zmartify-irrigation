function(zic_add_host_test target test_source)
    add_executable(${target} ${test_source} ${ARGN})
    target_compile_features(${target} PRIVATE c_std_11)
    target_compile_options(${target} PRIVATE -Wall -Wextra -Werror)
    add_test(NAME ${target} COMMAND ${target})
endfunction()

zic_add_host_test(test_state_machine
    test_state_machine.c
    ../components/zic_core/src/zic_state_machine.c)
target_include_directories(test_state_machine PRIVATE ../components/zic_core/include)

zic_add_host_test(test_safety_rules
    test_safety_rules.c
    ../components/alarm_manager/src/alarm_manager.c
    ../components/flow_manager/src/flow_manager.c
    ../components/pressure_manager/src/pressure_manager.c)
target_include_directories(test_safety_rules PRIVATE
    ../components/alarm_manager/include
    ../components/flow_manager/include
    ../components/pressure_manager/include)

zic_add_host_test(test_config_validation
    test_config_validation.c
    ../components/config_manager/src/config_validation.c)
target_include_directories(test_config_validation PRIVATE
    ../components/config_manager/include)
target_link_libraries(test_config_validation PRIVATE m)

zic_add_host_test(test_valve_diagnostics
    test_valve_diagnostics.c
    ../components/valve_diagnostics/src/valve_diagnostics.c)
target_include_directories(test_valve_diagnostics PRIVATE
    ../components/valve_diagnostics/include)

zic_add_host_test(test_irrigation_engine
    test_irrigation_engine.c
    ../components/irrigation_engine/src/irrigation_engine.c
    ../components/zone_manager/src/zone_manager.c
    ../components/zic_core/src/zic_state_machine.c)
target_include_directories(test_irrigation_engine PRIVATE
    ../components/irrigation_engine/include
    ../components/relay_manager/include
    ../components/zone_manager/include
    ../components/zic_core/include)

zic_add_host_test(test_storage_manager
    test_storage_manager.c
    ../components/storage_manager/src/storage_manager.c)
target_include_directories(test_storage_manager PRIVATE ../components/storage_manager/include)

zic_add_host_test(test_watersensor_protocol
    test_watersensor_protocol.c
    ../components/watersensor_client/src/watersensor_protocol.c)
target_include_directories(test_watersensor_protocol PRIVATE ../components/watersensor_client/include)

zic_add_host_test(test_weather_manager
    test_weather_manager.c
    ../components/weather_manager/src/weather_manager.c)
target_include_directories(test_weather_manager PRIVATE ../components/weather_manager/include)

zic_add_host_test(test_et_engine
    test_et_engine.c
    ../components/et_engine/src/et_engine.c)
target_include_directories(test_et_engine PRIVATE ../components/et_engine/include)
target_link_libraries(test_et_engine PRIVATE m)

zic_add_host_test(test_ota_policy
    test_ota_policy.c
    ../components/ota_manager/src/ota_policy.c)
target_include_directories(test_ota_policy PRIVATE ../components/ota_manager/include)

zic_add_host_test(test_acceptance_safety_shutdown
    test_acceptance_safety_shutdown.c
    ../components/alarm_manager/src/alarm_manager.c
    ../components/irrigation_engine/src/irrigation_engine.c
    ../components/pressure_manager/src/pressure_manager.c
    ../components/storage_manager/src/storage_manager.c
    ../components/zone_manager/src/zone_manager.c
    ../components/zic_core/src/zic_state_machine.c)
target_include_directories(test_acceptance_safety_shutdown PRIVATE
    ../components/alarm_manager/include
    ../components/irrigation_engine/include
    ../components/pressure_manager/include
    ../components/relay_manager/include
    ../components/storage_manager/include
    ../components/zone_manager/include
    ../components/zic_core/include)