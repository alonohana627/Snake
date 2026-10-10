# The game logic: rules and state only. Depends on nothing - no windows, no
# Vulkan, not even core's pixel/color types - so it's easy to test.
snake_add_component(game
    game.cpp
    snake.cpp
)
