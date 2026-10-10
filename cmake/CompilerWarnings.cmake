# Apply the project's warning set to a target.
function(snake_set_warnings target)
    if(MSVC)
        target_compile_options(${target} PRIVATE /W4 /permissive- /Zc:__cplusplus /utf-8)
    else()
        target_compile_options(${target} PRIVATE
            -Wall -Wextra -Wpedantic
            -Wconversion -Wshadow
            -Wnon-virtual-dtor -Wold-style-cast -Woverloaded-virtual
        )
    endif()
endfunction()
