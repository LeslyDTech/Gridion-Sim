# gridiron_set_warnings(<target>)
#
# Applies strict warnings to one of *our* targets. Third-party targets are left alone,
# and their headers are pulled in as SYSTEM so they don't produce warnings in our code.
function(gridiron_set_warnings target)
    if(MSVC)
        set(warnings
            /W4          # high warning level
            /permissive- # strict standard conformance
        )
        if(GRIDIRON_WARNINGS_AS_ERRORS)
            list(APPEND warnings /WX)
        endif()
    else()
        set(warnings
            -Wall
            -Wextra
            -Wpedantic
            -Wshadow             # a local variable hides another one
            -Wnon-virtual-dtor
            -Wold-style-cast
        )
        if(GRIDIRON_WARNINGS_AS_ERRORS)
            list(APPEND warnings -Werror)
        endif()
    endif()

    target_compile_options(${target} PRIVATE ${warnings})
endfunction()
