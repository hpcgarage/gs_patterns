cmake_minimum_required(VERSION 3.1...3.25)

set (CMAKE_VERBOSE_MAKEFILE "1")

project( gs_patterns VERSION 1.0 LANGUAGES CXX)

add_executable(gs_patterns gs_patterns.cpp)

set(CMAKE_CXX_STANDARD_LIBRARIES "-lm -lz ${CMAKE_CXX_STANDARD_LIBRARIES}")
