#pragma once

#include "snake/core/fixed_timestep.hpp"
#include "snake/game/game.hpp"
#include "snake/gfx/renderer.hpp"
#include "snake/platform/window.hpp"

namespace snake::app {

// Everything around the game: opens the window, runs the main loop, turns key
// presses into game commands, ticks the game on time and draws it.
class Application {
public:
    Application();

    Application(const Application&) = delete;
    Application& operator=(const Application&) = delete;

    // Runs until the window is closed (or Escape is pressed).
    void run();

private:
    void handleInput();
    void updateTitle();

    // Members are created top to bottom and destroyed bottom to top:
    // the renderer goes before the window it draws into.
    platform::Window m_window;
    gfx::Renderer m_renderer;
    game::Game m_game;
    core::FixedTimestep m_ticks;

    // What the window title shows right now, so it's only changed when needed.
    int m_shownScore = -1;
    int m_shownBestScore = -1;
};

} // namespace snake::app
