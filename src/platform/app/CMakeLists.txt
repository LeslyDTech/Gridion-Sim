# gridiron_app: the raylib-free part of the application layer (scene stack, fade transitions,
# settings). Kept separate from the raylib executable so it can be unit tested headless, the same
# way gridiron_core is.
add_library(gridiron_app STATIC
    SceneManager.cpp
    SceneStack.cpp
    Settings.cpp
)

# Consumers write #include "app/SceneStack.h"; src/platform is the include root here.
target_include_directories(gridiron_app PUBLIC ${PROJECT_SOURCE_DIR}/src/platform)
target_link_libraries(gridiron_app PRIVATE nlohmann_json::nlohmann_json)
gridiron_set_warnings(gridiron_app)
