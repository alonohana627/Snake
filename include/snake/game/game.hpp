#pragma once

#include "snake/core/types.hpp"

#include <vector>

namespace snake::game {

// The game advances in fixed steps ("ticks"), independent of the frame rate.
// A 60 Hz and a 144 Hz monitor both run exactly kTicksPerSecond ticks per second.
inline constexpr int kTicksPerSecond = 60;
inline constexpr double kTickSeconds = 1.0 / kTicksPerSecond;

// What the player wants to do this tick. Each axis is -1, 0 or +1.
struct Input {
    float moveX = 0.0f;
    float moveY = 0.0f;
};

// All game state and rules. Knows nothing about windows or Vulkan:
// it gets input and the play area, and describes what there is to draw.
class Game {
public:
    // Starts with the player in the middle of the play area.
    explicit Game(core::Size area);

    // Advance the game by exactly one tick (kTickSeconds).
    void tick(const Input& input, core::Size area);

    // Everything to draw this frame, back to front.
    std::vector<core::ColoredRect> drawList() const;

    const core::Rect& player() const { return m_player; }

private:
    core::Rect m_player;
};

} // namespace snake::game
