#pragma once

#include "snake/core/types.hpp"
#include "snake/game/game.hpp"

#include <vector>

namespace snake::app {

// How the game looks: turns the game's state (cells) into sprites to draw
// (pixels), laid out to fit `area`. Back to front: the board, then the snake.
std::vector<core::Sprite> gameDrawList(const game::Game& game, core::Size area);

} // namespace snake::app
