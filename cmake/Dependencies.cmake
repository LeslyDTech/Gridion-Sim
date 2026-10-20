# Third-party dependencies, downloaded at configure time with FetchContent.
#
# Every dependency is pinned to an exact release so builds are reproducible.
# Release tarballs are used instead of `git clone` because they download faster
# and don't pull the full repository history.
include(FetchContent)

# nlohmann/json - header-only JSON parser used by the play loader (core).
# The json.tar.xz release asset is the officially recommended FetchContent source.
FetchContent_Declare(nlohmann_json
    URL https://github.com/nlohmann/json/releases/download/v3.12.0/json.tar.xz
    SYSTEM
)
FetchContent_MakeAvailable(nlohmann_json)

if(GRIDIRON_BUILD_GAME)
    # raylib - windowing, input and 2D rendering (platform layer only).
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_GAMES OFF CACHE BOOL "" FORCE)
    FetchContent_Declare(raylib
        URL https://github.com/raysan5/raylib/archive/refs/tags/5.5.tar.gz
        SYSTEM
    )
    FetchContent_MakeAvailable(raylib)
endif()

if(GRIDIRON_BUILD_TESTS)
    # Catch2 v3 - unit test framework.
    FetchContent_Declare(Catch2
        URL https://github.com/catchorg/Catch2/archive/refs/tags/v3.8.1.tar.gz
        SYSTEM
    )
    FetchContent_MakeAvailable(Catch2)
    # Makes `include(Catch)` / catch_discover_tests() available to tests/CMakeLists.txt.
    list(APPEND CMAKE_MODULE_PATH ${catch2_SOURCE_DIR}/extras)
endif()
