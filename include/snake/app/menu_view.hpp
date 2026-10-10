#pragma once

#include "snake/app/menu.hpp"
#include "snake/app/settings.hpp"
#include "snake/core/types.hpp"

#include <vector>

namespace snake::app {

// How the menu looks: darkens whatever is already in `sprites` (the paused game)
// and adds a panel with the menu's title and items on top, centered on the canvas.
void appendMenu(std::vector<core::Sprite>& sprites, const Menu& menu, const Settings& settings, core::Size canvas);

} // namespace snake::app
