cmake_minimum_required(VERSION 3.18)

project(Utility LANGUAGES CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_library(Utility INTERFACE)
file(GLOB HEADERS ${PROJECT_SOURCE_DIR}/*.h)
target_sources(Utility INTERFACE ${HEADERS})
target_include_directories(Utility INTERFACE ${PROJECT_SOURCE_DIR})

add_executable(test test.cpp)
target_link_libraries(test PUBLIC Utility)
