# gridiron: the playable executable. The only target that links raylib.
add_executable(gridiron
    main.cpp
)

target_link_libraries(gridiron PRIVATE gridiron_core raylib)
gridiron_set_warnings(gridiron)

# Copy data/ (play JSON files) next to the executable after every build, so the game finds
# it no matter which directory it is launched from.
add_custom_command(TARGET gridiron POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            ${PROJECT_SOURCE_DIR}/data $<TARGET_FILE_DIR:gridiron>/data
    COMMENT "Copying data/ next to the executable"
)
