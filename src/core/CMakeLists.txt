# gridiron_core: all game rules and simulation. Must never include raylib, so it can be
# unit tested headless and reused with any renderer.
add_library(gridiron_core STATIC
    Ball.cpp
    Collision.cpp
    FixedTimestep.cpp
    Game.cpp
    PlayerAi.cpp
    PlayerState.cpp
    PlayLoader.cpp
    Route.cpp
    SpatialHash.cpp
    Steering.cpp
    World.cpp
)

# Consumers write #include "core/Vec2.h"; the src/ directory is the include root.
target_include_directories(gridiron_core PUBLIC ${PROJECT_SOURCE_DIR}/src)

# PRIVATE: only PlayLoader.cpp uses JSON; no core header exposes nlohmann types.
target_link_libraries(gridiron_core PRIVATE nlohmann_json::nlohmann_json)

gridiron_set_warnings(gridiron_core)
