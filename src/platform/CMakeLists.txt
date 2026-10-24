# gridiron: the playable executable. The only target that links raylib.
add_executable(gridiron
    main.cpp
    App.cpp
    AttractMode.cpp
    AssetDecode.cpp
    AssetLoader.cpp
    Assets.cpp
    BroadcastHud.cpp
    DebugOverlay.cpp
    Effects.cpp
    FieldRenderer.cpp
    FollowCamera.cpp
    GameScene.cpp
    Hud.cpp
    InputDevice.cpp
    LoadingScene.cpp
    MenuWidgets.cpp
    PauseScene.cpp
    PlayerSprites.cpp
    Renderer.cpp
    TitleScene.cpp
    UiDraw.cpp
)

target_link_libraries(gridiron PRIVATE gridiron_core gridiron_app raylib)
gridiron_set_warnings(gridiron)

# Copy data/ (play JSON files) and assets/ (fonts) next to the executable after every build,
# so the game finds them no matter which directory it is launched from.
add_custom_command(TARGET gridiron POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${PROJECT_SOURCE_DIR}/data $<TARGET_FILE_DIR:gridiron>/data
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${PROJECT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:gridiron>/assets
    COMMENT "Copying data/ and assets/ next to the executable"
)
