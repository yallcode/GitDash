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

file(GLOB_RECURSE SOURCES src/*.cpp src/*.hpp)
add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

# Link zlib — available on all platforms via the NDK / system / Geode deps
if(ANDROID)
    target_link_libraries(${PROJECT_NAME} z)
elseif(WIN32)
    # Geode on Windows bundles zlib — link it from the SDK
    target_link_libraries(${PROJECT_NAME} "${GEODE_SDK}/loader/include/link/zlib.lib")
else()
    find_package(ZLIB REQUIRED)
    target_link_libraries(${PROJECT_NAME} ZLIB::ZLIB)
endif()

setup_geode_mod(${PROJECT_NAME})
