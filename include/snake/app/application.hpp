#pragma once

#include "snake/app/menu.hpp"
#include "snake/app/save_data.hpp"
#include "snake/app/settings.hpp"
#include "snake/app/sounds.hpp"
#include "snake/audio/player.hpp"
#include "snake/core/fixed_timestep.hpp"
#include "snake/game/game.hpp"
#include "snake/gfx/renderer.hpp"
#include "snake/platform/window.hpp"

#include <filesystem>

namespace snake::app {

// Everything around the game: opens the window, runs the main loop, routes key
// presses to the menu or the game, ticks the game on time, draws it and plays its sounds.
class Application {
public:
    Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Runs until the window is closed (or Quit is picked in the menu).
    void run();

private:
    // What's on screen. The game only ticks while Playing; in the menu it's
    // paused and drawn behind it.
    enum class Screen {
        Menu,
        Playing,
    };

    void handleInput();
    void handleMenuKey(platform::Key key);
    void handleMenuCommand(MenuCommand command);
    void openMenu();
    void play();
    void applySettings();
    void save();

    // Members are created top to bottom and destroyed bottom to top:
    // the renderer goes before the window it draws into.
    platform::Window m_window;
    gfx::Renderer m_renderer;
    audio::Player m_player;
    Sounds m_sounds; // uses m_player, so it comes after it
    game::Game m_game;
    Settings m_settings;
    core::FixedTimestep m_ticks;
    Menu m_menu;
    Screen m_screen = Screen::Menu;
    bool m_hasStarted = false; // a game has been started, so the menu can offer Continue

    std::filesystem::path m_savePath;
    SaveData m_saved; // what the file has now, so unchanged data isn't written again
};

} // namespace snake::app
