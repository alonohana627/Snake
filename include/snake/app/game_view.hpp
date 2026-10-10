#pragma once

#include "snake/core/types.hpp"
#include "snake/game/game.hpp"

#include <vector>

namespace snake::app {

// How the game looks: turns the game's state (cells) into sprites to draw
// (pixels), laid out on a canvas of `canvas` pixels. Back to front: the
// background, the board, the food, the snake, then the side panels (score on
// the left, food values and controls on the right).
std::vector<core::Sprite> gameDrawList(const game::Game& game, core::Size canvas);

} // namespace snake::app
