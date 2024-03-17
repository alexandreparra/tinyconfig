cmake_minimum_required(VERSION 3.25)
project(tinyconfig C)

set(CMAKE_C_STANDARD 17)

add_library(tinyconfig STATIC src/tinyconfig.c include/tinyconfig.h)
set_target_properties(tinyconfig PROPERTIES PREFIX "")
target_include_directories(tinyconfig PUBLIC include/)
