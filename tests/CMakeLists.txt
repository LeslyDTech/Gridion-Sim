# gridiron_tests links gridiron_core but NOT raylib, which proves the game logic builds and
# runs headless.
add_executable(gridiron_tests
    test_drive.cpp
    test_fixed_timestep.cpp
    test_play_loader.cpp
    test_player_state.cpp
    test_route.cpp
    test_spatial_hash.cpp
    test_world.cpp
)

target_link_libraries(gridiron_tests PRIVATE gridiron_core Catch2::Catch2WithMain)
gridiron_set_warnings(gridiron_tests)

# Lets tests read the real play files in data/ without depending on the working directory.
target_compile_definitions(gridiron_tests PRIVATE
    GRIDIRON_DATA_DIR="${PROJECT_SOURCE_DIR}/data")

# Registers each TEST_CASE as its own CTest test, so failures are reported by name.
include(Catch)
catch_discover_tests(gridiron_tests)
