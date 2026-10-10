#pragma once

#include "snake/game/grid.hpp"

#include <cstddef>
#include <deque>

namespace snake::game {

// The snake: a chain of grid cells, head first, that moves one cell per step.
// It only knows about itself - walls, food and the board are the game's job.
class Snake {
public:
    // A straight snake of `length` cells (at least 1) with its head at `head`,
    // facing `direction`. The body trails behind it.
    Snake(Cell head, int length, Direction direction);

    // Ask to turn. Turns are queued and used one per step, in order, so quick
    // presses between two steps (e.g. Down then Left for a tight U-turn) all
    // count. Ignored: turning straight back (e.g. Left while heading Right),
    // repeating the same direction, and anything past kMaxQueuedTurns.
    void turn(Direction direction);

    // Where the head goes on the next step: one cell in the next direction.
    Cell nextHead() const { return neighbor(head(), nextDirection()); }

    // Move one cell forward: a new head in front, the tail cell dropped -
    // unless the snake is growing, then the tail stays where it is.
    void step() { stepTo(nextHead()); }

    // Like step(), but the new head goes to `newHead` instead of nextHead().
    // For the game to apply board rules, e.g. wrapping around at the edges.
    void stepTo(Cell newHead);

    // Grow by `cells`: the tail stays put for that many upcoming steps.
    void grow(int cells = 1);

    Cell head() const { return m_body.front(); }
    const std::deque<Cell>& body() const { return m_body; } // head first, tail last
    int length() const { return static_cast<int>(m_body.size()); }

    // The direction of the last step (what the snake is visibly doing).
    Direction direction() const { return m_direction; }

    bool occupies(Cell cell) const;

    // True if the head is on one of the snake's own body cells.
    bool hasBittenItself() const;

private:
    // Enough for quick combos, small enough that old presses don't linger for ages.
    static constexpr std::size_t kMaxQueuedTurns = 3;

    // The direction the next step will use: the first queued turn, if any.
    Direction nextDirection() const { return m_turns.empty() ? m_direction : m_turns.front(); }

    std::deque<Cell> m_body;      // front = head, back = tail
    Direction m_direction;        // the direction of the last step
    std::deque<Direction> m_turns; // turns waiting for upcoming steps, oldest first
    int m_pendingGrowth = 0;      // steps left during which the tail stays put
};

} // namespace snake::game
