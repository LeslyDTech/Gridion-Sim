# Gridiron Sim - top-level build script.
#
# Layout:
#   src/core      -> static library `gridiron_core` (pure C++, no raylib)
#   src/platform  -> executable `gridiron` (raylib window, input, rendering)
#   tests         -> Catch2 executable `gridiron_tests` (links core only)
#
# 3.25 is the minimum because FetchContent_Declare(... SYSTEM) needs it; SYSTEM keeps
# third-party headers from triggering our strict warning flags.
cmake_minimum_required(VERSION 3.25)

project(GridironSim
    VERSION 0.1.0
    DESCRIPTION "Top-down 2D American football play simulator"
    LANGUAGES C CXX) # C is required because raylib is written in C.

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF) # -std=c++17, not -std=gnu++17

# Put every executable in build/bin (multi-config generators such as Visual Studio
# append the configuration, e.g. build/bin/Release).
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

option(GRIDIRON_BUILD_GAME "Build the raylib executable" ON)
option(GRIDIRON_BUILD_TESTS "Build the Catch2 unit tests" ON)
option(GRIDIRON_WARNINGS_AS_ERRORS "Treat compiler warnings as errors (used by CI)" OFF)

# Browser build (configured with `emcmake cmake`, see README): the game compiled to WebAssembly.
# The tests run in the native builds, so they're skipped here.
if(EMSCRIPTEN)
    set(GRIDIRON_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    # raylib's web backend on WebGL 2 (OpenGL ES 3.0): unlike WebGL 1 it can mipmap textures whose
    # sizes aren't powers of two, which the player atlas and font atlases rely on.
    set(PLATFORM "Web" CACHE STRING "" FORCE)
    set(OPENGL_VERSION "ES 3.0" CACHE STRING "" FORCE)
    # Emscripten turns C++ exceptions off by default; the play loader reports bad JSON with one.
    add_compile_options(-fexceptions)
    add_link_options(-fexceptions)
endif()

list(APPEND CMAKE_MODULE_PATH ${PROJECT_SOURCE_DIR}/cmake)
include(CompilerWarnings)
include(Dependencies)

add_subdirectory(src/core)
add_subdirectory(src/platform/app) # raylib-free app layer (scenes, settings): always built, tested

if(GRIDIRON_BUILD_GAME)
    add_subdirectory(src/platform)
endif()

if(GRIDIRON_BUILD_TESTS)
    enable_testing()
    add_subdirectory(tests)
endif()
