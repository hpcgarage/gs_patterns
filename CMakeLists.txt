cmake_minimum_required(VERSION 3.1...3.25)

set (CMAKE_VERBOSE_MAKEFILE "1")

project(
  gs_patterns
  VERSION 1.0
  LANGUAGES C)

add_executable(gs_patterns gs_patterns.c)

#set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -lm -lz")
#set(CMAKE_LINK_LIBRARY_FLAG "-lz -lm")
#set(CMAKE_C_LINK_LIBRARY_FLAG "-lm -lz")
set(CMAKE_C_STANDARD_LIBRARIES "-lm -lz ${CMAKE_C_STANDARD_LIBRARIES}")

#set (CMAKE_C_FLAGS "-lm -lz")
