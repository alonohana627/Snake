#pragma once

namespace snake::game {

// A position on the game grid, in cells (not pixels). x grows to the right,
// y grows downwards - the same way as on screen.
struct Cell {
    int x = 0;
    int y = 0;

    friend constexpr bool operator==(Cell, Cell) = default;
};

enum class Direction {
    Up,
    Down,
    Left,
    Right,
};

// The cell next to `cell`, one step in `direction`.
constexpr Cell neighbor(Cell cell, Direction direction)
{
    switch (direction) {
    case Direction::Up:    return {cell.x, cell.y - 1};
    case Direction::Down:  return {cell.x, cell.y + 1};
    case Direction::Left:  return {cell.x - 1, cell.y};
    case Direction::Right: return {cell.x + 1, cell.y};
    }
    return cell;
}

constexpr Direction opposite(Direction direction)
{
    switch (direction) {
    case Direction::Up:    return Direction::Down;
    case Direction::Down:  return Direction::Up;
    case Direction::Left:  return Direction::Right;
    case Direction::Right: return Direction::Left;
    }
    return direction;
}

// Bring a cell that left a width x height board back in on the opposite side.
// (In C++, -1 % 20 is -1, so add the size once more to get 19.)
constexpr Cell wrap(Cell cell, int width, int height)
{
    return {((cell.x % width) + width) % width, ((cell.y % height) + height) % height};
}

} // namespace snake::game
