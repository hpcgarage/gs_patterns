cmake_minimum_required(VERSION 3.1...3.25)

set (CMAKE_VERBOSE_MAKEFILE "1")

project( gs_patterns VERSION 1.0 LANGUAGES CXX)

add_executable(
        gs_patterns
        utils.h
        utils.cpp
        gs_patterns.h
        gs_patterns.cpp
        gs_patterns_core.h
        gs_patterns_core.cpp
)

set(CMAKE_CXX_STANDARD_LIBRARIES "-lm -lz ${CMAKE_CXX_STANDARD_LIBRARIES}")
