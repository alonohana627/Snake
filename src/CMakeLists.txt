find_package(Vulkan REQUIRED COMPONENTS glslc)

# One static library per component (see cmake/SnakeComponent.cmake).
#
#   snake (main.cpp)    creates the app and runs it
#   └── app             main loop, menu, key -> game command, how the game looks and sounds
#       ├── game        the rules - depends on nothing at all
#       ├── gfx         Vulkan renderer (uses platform for the window's surface)
#       ├── platform    window + keyboard (GLFW)
#       ├── audio       8-bit sound synth + playback (miniaudio)
#       ├── storage     files in the user's data folder, safe writes, INI format
#       └── core        shared types and FixedTimestep (used by app, gfx, platform)

add_subdirectory(core)
add_subdirectory(platform)
add_subdirectory(gfx)
add_subdirectory(audio)
add_subdirectory(storage)
add_subdirectory(game)
add_subdirectory(app)

# The executable just creates the app and runs it.
add_executable(snake
    main.cpp
)
snake_set_warnings(snake)
target_link_libraries(snake PRIVATE snake::app)
