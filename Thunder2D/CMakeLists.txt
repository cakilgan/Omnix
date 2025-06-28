cmake_minimum_required(VERSION 3.31.0)
project(Thunder2D VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)  
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -O0 -g3")

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_library(Thunder2D ${SOURCES})
target_include_directories(Thunder2D PUBLIC "include")
target_link_libraries(Thunder2D PUBLIC OmnixLib)

