cmake_minimum_required(VERSION 3.25)
project(tinyconfig C)

set(CMAKE_C_STANDARD 17)

add_library(tinyconfig STATIC src/tinyconfig.c include/tinyconfig.h)
target_include_directories(tinyconfig PUBLIC include/)
