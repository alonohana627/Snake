find_package(Vulkan REQUIRED COMPONENTS glslc)

# One static library per component (see cmake/SnakeComponent.cmake).
#
#            snake (main.cpp)
#           /     |        \
#       game     gfx ----> platform
#           \     |        /
#               core
add_subdirectory(core)
add_subdirectory(platform)
add_subdirectory(gfx)
add_subdirectory(game)

# The executable only wires the components together.
add_executable(snake
    main.cpp
)
snake_set_warnings(snake)
target_link_libraries(snake PRIVATE snake::core snake::platform snake::gfx snake::game)
