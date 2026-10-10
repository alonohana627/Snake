#include "snake/game/snake.hpp"

#include <algorithm>
#include <cassert>
#include <iterator>

namespace snake::game {

Snake::Snake(Cell head, int length, Direction direction)
    : m_direction(direction)
{
    assert(length >= 1);

    // Lay the body out behind the head, opposite to where it faces.
    Cell cell = head;
    for (int i = 0; i < length; ++i) {
        m_body.push_back(cell);
        cell = neighbor(cell, opposite(direction));
    }
}

void Snake::turn(Direction direction)
{
    // Check against the direction the snake will have when this turn is used:
    // the last queued turn, or its current direction if none are queued.
    // So moving Right, Up then Left is a valid U-turn (Up first, Left next step),
    // but Up then Down is ignored (it would reverse onto itself).
    const Direction before = m_turns.empty() ? m_direction : m_turns.back();
    if (direction == before || direction == opposite(before) || m_turns.size() >= kMaxQueuedTurns) {
        return;
    }
    m_turns.push_back(direction);
}

void Snake::stepTo(Cell newHead)
{
    m_direction = nextDirection();
    if (!m_turns.empty()) {
        m_turns.pop_front();
    }
    m_body.push_front(newHead);

    if (m_pendingGrowth > 0) {
        --m_pendingGrowth;
    } else {
        m_body.pop_back();
    }
}

void Snake::grow(int cells)
{
    assert(cells >= 0);
    m_pendingGrowth += cells;
}

bool Snake::occupies(Cell cell) const
{
    return std::ranges::find(m_body, cell) != m_body.end();
}

bool Snake::hasBittenItself() const
{
    // Look for the head among the other cells (skip the head itself).
    return std::find(std::next(m_body.begin()), m_body.end(), head()) != m_body.end();
}

} // namespace snake::game
