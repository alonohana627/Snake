# Game state and rules. Only core: no windows, no Vulkan - easy to test.
snake_add_component(game
    game.cpp
)
target_link_libraries(snake_game PUBLIC snake::core)
