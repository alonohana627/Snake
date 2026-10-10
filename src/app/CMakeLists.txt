# Everything around the game logic: the main loop, key -> game command mapping,
# and how the game looks on screen. Ties all the other components together.
snake_add_component(app
    application.cpp
    game_view.cpp
)
target_link_libraries(snake_app
    PUBLIC snake::core snake::platform snake::gfx snake::game
)
