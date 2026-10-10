# Vulkan rendering. Uses platform only to get the window's surface and size.
snake_add_component(gfx
    check.cpp
    device.cpp
    frame.cpp
    instance.cpp
    renderer.cpp
    sprite_pipeline.cpp
    surface.cpp
    swapchain.cpp
)
target_link_libraries(snake_gfx
    PUBLIC snake::core Vulkan::Vulkan
    PRIVATE snake::platform
)

# Shaders: GLSL -> SPIR-V at build time. glslc's "-mfmt=num" writes the SPIR-V as
# a comma-separated list of numbers, which sprite_pipeline.cpp #includes into an array.
# That way the shaders are baked into snake.exe - no files to load at runtime.
set(SHADERS
    shaders/sprite.vert
    shaders/rect.frag
    shaders/snake_head.frag
    shaders/food.frag
    shaders/background.frag
    shaders/glyph.frag
)
foreach(shader IN LISTS SHADERS)
    get_filename_component(name ${shader} NAME)
    set(output ${CMAKE_CURRENT_BINARY_DIR}/${name}.spv.inc)
    add_custom_command(
        OUTPUT ${output}
        COMMAND Vulkan::glslc --target-env=vulkan1.3 -mfmt=num -o ${output} ${CMAKE_CURRENT_SOURCE_DIR}/${shader}
        DEPENDS ${shader}
        COMMENT "Compiling shader ${name}"
        VERBATIM
    )
    target_sources(snake_gfx PRIVATE ${output})
endforeach()
target_include_directories(snake_gfx PRIVATE ${CMAKE_CURRENT_BINARY_DIR})
