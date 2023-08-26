cmake_minimum_required(VERSION 3.25)
project(tinyconfig_example C)

set(CMAKE_C_STANDARD 17)

add_executable(tinyconf_example main.c ../src/tinyconfig.c ../include/tinyconfig.h)
target_include_directories(tinyconf_example PUBLIC ../include)
