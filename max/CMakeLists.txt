cmake_minimum_required(VERSION 3.31.0)
project(max VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug)  
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -Wall -Wextra -O0 -g3")

file(GLOB_RECURSE SOURCES "src/*.cpp")
add_library(max ${SOURCES})
target_link_libraries(max OmnixLib)
target_include_directories(max PUBLIC
"include"
${CMAKE_CURRENT_SOURCE_DIR}/include
${CMAKE_CURRENT_SOURCE_DIR}/CakilganCore/include
${CMAKE_CURRENT_SOURCE_DIR}/glad/include
${CMAKE_CURRENT_SOURCE_DIR}/max/include
${CMAKE_CURRENT_SOURCE_DIR}/brain/include
${CMAKE_CURRENT_SOURCE_DIR}/Thunder2D/include
${CMAKE_CURRENT_SOURCE_DIR}/freetype/include
)

