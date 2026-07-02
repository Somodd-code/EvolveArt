cmake_minimum_required(VERSION 3.21)
project(EvolveArt VERSION 0.1.0)

# --- Fetch Geode SDK ---
# If you already have geode-cli set up and run `geode new` this block is
# normally generated for you. Left here so the project builds standalone too.
if (NOT DEFINED ENV{GEODE_SDK})
    message(FATAL_ERROR "Please define GEODE_SDK environment variable (path to your Geode SDK checkout). Install the Geode CLI first: https://docs.geode-sdk.org/getting-started/")
endif()

# The current Visual Studio toolchain crashes while compiling Geode's
# generated precompiled headers for this project, so disable them here.
if (NOT DEFINED GEODE_DISABLE_PRECOMPILED_HEADERS)
    set(GEODE_DISABLE_PRECOMPILED_HEADERS ON CACHE BOOL "Disable Geode precompiled headers to avoid MSVC crashes" FORCE)
endif()

add_subdirectory($ENV{GEODE_SDK} $ENV{GEODE_SDK}/build)

file(GLOB SOURCES CONFIGURE_DEPENDS
    src/*.cpp
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_include_directories(${PROJECT_NAME} PRIVATE src)

setup_geode_mod(${PROJECT_NAME})
