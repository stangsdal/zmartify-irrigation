cmake_minimum_required(VERSION 3.16)

option(ZIC_HOST_TESTS "Build host-side unit and acceptance tests" OFF)

if(ZIC_HOST_TESTS)
	project(zmartify_irrigation_host_tests C)
	enable_testing()
	add_subdirectory(test)
	return()
endif()

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(zmartify_irrigation)
