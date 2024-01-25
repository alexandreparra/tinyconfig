cmake_minimum_required(VERSION 3.25)
project(tinyconfig_tests C)

set(CMAKE_C_STANDARD 17)

add_compile_definitions(TC_CONFIG_DEFAULT_SIZE=6)
add_compile_definitions(TC_DEBUG_LOGS)

add_executable(tinyconfig_tests
    main.c 
    ../src/tinyconfig.c ../include/tinyconfig.h
)
target_include_directories(tinyconfig_tests PUBLIC ../include)
