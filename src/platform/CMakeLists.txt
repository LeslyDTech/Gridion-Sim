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
    SettingsStore.cpp
    TitleScene.cpp
    UiDraw.cpp
)

target_link_libraries(gridiron PRIVATE gridiron_core gridiron_app raylib)
gridiron_set_warnings(gridiron)

if(EMSCRIPTEN)
    # The page is index.html, so GitHub Pages serves it at the site root. data/ and assets/ are
    # packed into index.data, which the page downloads before the game starts, and appear at
    # /data and /assets in Emscripten's virtual file system.
    set_target_properties(gridiron PROPERTIES OUTPUT_NAME "index" SUFFIX ".html")
    target_link_options(gridiron PRIVATE
        # Quoted: the project path may contain spaces.
        "SHELL:--preload-file \"${PROJECT_SOURCE_DIR}/data@/data\""
        "SHELL:--preload-file \"${PROJECT_SOURCE_DIR}/assets@/assets\""
        "SHELL:--shell-file \"${PROJECT_SOURCE_DIR}/web/shell.html\""
        "-sALLOW_MEMORY_GROWTH=1" # the decoded player atlas alone is 33 MB
        "-sSTACK_SIZE=1048576"
        "-sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=$stringToNewUTF8" # used by SettingsStore.cpp
    )
    set_property(TARGET gridiron APPEND PROPERTY LINK_DEPENDS ${PROJECT_SOURCE_DIR}/web/shell.html)
    return()
endif()

# Copy data/ (play JSON files) and assets/ (fonts) next to the executable after every build,
# so the game finds them no matter which directory it is launched from.
add_custom_command(TARGET gridiron POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${PROJECT_SOURCE_DIR}/data $<TARGET_FILE_DIR:gridiron>/data
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${PROJECT_SOURCE_DIR}/assets $<TARGET_FILE_DIR:gridiron>/assets
    COMMENT "Copying data/ and assets/ next to the executable"
)
