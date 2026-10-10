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

    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        # Vulkan code relies on designated initializers leaving out fields on purpose
        # (VkFooCreateInfo{.sType = ..., .size = ...}): the rest are zero, which is the
        # right default. Plain {a, b} initializers missing fields are still warned about.
        target_compile_options(${target} PRIVATE -Wno-missing-designated-field-initializers)
    endif()
endfunction()
