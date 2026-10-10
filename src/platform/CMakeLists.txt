# Window + keyboard. The only component that knows GLFW (linked PRIVATE, so it
# doesn't leak to anyone using platform). window.hpp uses Vulkan handle types,
# hence the public Vulkan headers.
snake_add_component(platform
    window.cpp
)
target_link_libraries(snake_platform
    PUBLIC snake::core Vulkan::Headers
    PRIVATE glfw
)
