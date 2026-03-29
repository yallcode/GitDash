cmake_minimum_required(VERSION 3.21)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_ARCHITECTURES "x86_64")

set(PROJECT_NAME GitDash)
project(${PROJECT_NAME} VERSION 1.0)

if(DEFINED ENV{GEODE_SDK})
    set(GEODE_SDK $ENV{GEODE_SDK})
endif()

add_subdirectory(${GEODE_SDK} SYSTEM ${CMAKE_CURRENT_BINARY_DIR}/geode)

# Add nlohmann/json (not bundled in Geode 5.x)
CPMAddPackage("gh:nlohmann/json@3.11.3")

file(GLOB_RECURSE SOURCES src/*.cpp src/*.hpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

target_link_libraries(${PROJECT_NAME} nlohmann_json::nlohmann_json)

setup_geode_mod(${PROJECT_NAME})
