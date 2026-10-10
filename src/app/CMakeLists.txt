# Everything around the game logic: the main loop, the menu, key -> game command
# mapping, how the game and menu look on screen, and how they sound. Ties all the other components together.
snake_add_component(app
    application.cpp
    game_view.cpp
    menu.cpp
    menu_view.cpp
    save_data.cpp
    sounds.cpp
    text.cpp
)
target_link_libraries(snake_app
    PUBLIC snake::core snake::platform snake::gfx snake::audio snake::storage snake::game
)
