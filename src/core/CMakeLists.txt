# gridiron_core: all game rules and simulation. Must never include raylib, so it can be
# unit tested headless and reused with any renderer.
add_library(gridiron_core STATIC
    FixedTimestep.cpp
    PlayerState.cpp
    Route.cpp
    Steering.cpp
)

# Consumers write #include "core/Vec2.h"; the src/ directory is the include root.
target_include_directories(gridiron_core PUBLIC ${PROJECT_SOURCE_DIR}/src)

gridiron_set_warnings(gridiron_core)
