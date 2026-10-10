#pragma once

#include "snake/game/grid.hpp"
#include "snake/game/snake.hpp"

namespace snake::game {

// The game advances in fixed steps ("ticks"), independent of the frame rate.
// The snake moves one cell per tick, so this is also its speed in cells per second.
inline constexpr int kTicksPerSecond = 8;
inline constexpr double kTickSeconds = 1.0 / kTicksPerSecond;

// The board size, in cells.
inline constexpr int kBoardWidth = 20;
inline constexpr int kBoardHeight = 20;

// The game's rules and state - nothing else. It knows nothing about windows,
// keys, pixels, colors or time: it's told to turn and to tick, and others read
// its state to show it.
//
// Rules:
// - The snake moves one cell per tick and wraps around the board's edges.
// - It must not run into itself. If it does, it starts over from the start.
// - Score = whole seconds survived since the last start over.
class Game {
public:
    // A length-5 snake in the middle of the board, heading right.
    Game();

    // Ask the snake to turn. It happens on an upcoming tick.
    void turn(Direction direction);

    // Advance by exactly one tick: the snake moves one cell, and if it ran
    // into itself, the game starts over.
    void tick();

    const Snake& snake() const { return m_snake; }

    // Whole seconds survived since the last start over. Every tick is exactly
    // kTickSeconds long, so this comes from counting ticks - no clock needed.
    int score() const { return m_ticksSurvived / kTicksPerSecond; }

    // The highest score so far.
    int bestScore() const { return m_bestScore; }

private:
    void startOver();

    Snake m_snake;
    int m_ticksSurvived = 0;
    int m_bestScore = 0;
};

} // namespace snake::game
