cmake_minimum_required(VERSION 3.27)
project(dsl2_calculator)

set(CMAKE_CXX_STANDARD 23)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

include(sanitizers.cmake)
include(doctest.cmake)

enable_testing()

add_subdirectory(src)
