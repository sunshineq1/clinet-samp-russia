cmake_minimum_required(VERSION 3.12)
project(multiplayer)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_BUILD_TYPE Debug CACHE STRING "" FORCE)

if(${ANDROID_ABI} STREQUAL "armeabi-v7a")
    add_definitions(-DVER_x32=true)
elseif(${ANDROID_ABI} STREQUAL "arm64-v8a")
    add_definitions(-DVER_x32=false)
endif()

# Короткий хеш текущего коммита — он показывается в строке сборки поверх игры.
execute_process(
        COMMAND git rev-parse --short HEAD
        WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
        OUTPUT_VARIABLE WN_BUILD_HASH
        OUTPUT_STRIP_TRAILING_WHITESPACE
        ERROR_QUIET)
if(NOT WN_BUILD_HASH)
    set(WN_BUILD_HASH "dev")
endif()
add_definitions(-DWN_BUILD_HASH="${WN_BUILD_HASH}")

# Add source files
file(GLOB_RECURSE SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/*.c*
        ${CMAKE_CURRENT_LIST_DIR}/**/*.c*
        ${CMAKE_CURRENT_LIST_DIR}/**/**/*.c*
        ${CMAKE_CURRENT_LIST_DIR}/**/**/**/*.c*
        ${CMAKE_CURRENT_LIST_DIR}/**/**/**/**/*.c*
        ${CMAKE_CURRENT_LIST_DIR}/**/**/**/**/**/*.c*
        ${CMAKE_CURRENT_LIST_DIR}/**/**/**/**/**/**/*.c*
        ${CMAKE_CURRENT_LIST_DIR}/**/**/**/**/**/**/**/*.c*
        ${CMAKE_CURRENT_LIST_DIR}/**/**/**/**/**/**/**/**/*.c*
        ${CMAKE_CURRENT_LIST_DIR}/**/**/**/**/**/**/**/**/**/*.c*
        ${CMAKE_CURRENT_LIST_DIR}/**/**/**/**/**/**/**/**/**/**/*.c*
        ${CMAKE_CURRENT_LIST_DIR}/**/**/**/**/**/**/**/**/**/**/**/*.c*
)

# Add include directories
include_directories(
        ${CMAKE_CURRENT_LIST_DIR}
        ${CMAKE_CURRENT_LIST_DIR}/util/
        ${CMAKE_CURRENT_LIST_DIR}/vendor/armhook
        ${CMAKE_CURRENT_LIST_DIR}/vendor/imgui
        ${CMAKE_CURRENT_LIST_DIR}/vendor/
        ${CMAKE_CURRENT_LIST_DIR}/game/RW/
        ${CMAKE_CURRENT_LIST_DIR}/game/Core/
)

# Add RakNet files explicitly to avoid duplicates
file(GLOB RAKNET_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/vendor/raknet/*.cpp
        ${CMAKE_CURRENT_LIST_DIR}/vendor/raknet/SAMP/*.cpp
)

# Remove duplicates from SOURCES
list(REMOVE_DUPLICATES SOURCES)
list(REMOVE_DUPLICATES RAKNET_SOURCES)

# Combine sources
set(ALL_SOURCES ${SOURCES} ${RAKNET_SOURCES})

add_library(multiplayer SHARED ${ALL_SOURCES})

target_link_libraries(multiplayer
        log
        GLESv3
        EGL
        opus
        shadowhook::shadowhook
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/bass/libs/${ANDROID_ABI}/libbass.so
)

# Add RakNet files explicitly to avoid duplicates
file(GLOB RAKNET_SOURCES
        ${CMAKE_CURRENT_LIST_DIR}/vendor/raknet/*.cpp
        ${CMAKE_CURRENT_LIST_DIR}/vendor/raknet/SAMP/*.cpp
)

# Remove duplicates from SOURCES
list(REMOVE_DUPLICATES SOURCES)
list(REMOVE_DUPLICATES RAKNET_SOURCES)

# Combine sources
set(ALL_SOURCES ${SOURCES} ${RAKNET_SOURCES})

if(${ANDROID_ABI} STREQUAL "arm64-v8a")
    # page size increased for x64
    # 16kb+ support
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -Wl,-z,max-page-size=65536")
endif()

# Compilation flags for debug builds
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("- Build type: Debug - Selected!")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=default -Wall -Og -fexceptions -pthread -g3 -gdwarf-5")
endif()

# Compilation flags for release builds
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    message("- Build type: ${CMAKE_BUILD_TYPE} - Selected!")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fvisibility=hidden -s -w -O3 -fexceptions -fstack-protector-strong -pthread")

    if(${ANDROID_ABI} STREQUAL "arm64-v8a")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv8-a -mtune=cortex-a53")
    elseif(${ANDROID_ABI} STREQUAL "armeabi-v7a")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=armv7-a -mfpu=neon -mfloat-abi=softfp")
    endif()
endif()
