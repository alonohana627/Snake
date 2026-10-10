#include "snake/app/game_view.hpp"

#include <algorithm>
#include <ranges>

namespace snake::app {

namespace {

using game::Cell;
using game::Direction;
using game::kBoardHeight;
using game::kBoardWidth;

// The board is a checkerboard of two close shades, so the cells are visible.
constexpr core::Color kBoardColorA{.r = 0.08f, .g = 0.08f, .b = 0.10f, .a = 1.0f};
constexpr core::Color kBoardColorB{.r = 0.11f, .g = 0.11f, .b = 0.14f, .a = 1.0f};
constexpr core::Color kSnakeBodyColor{.r = 0.2f, .g = 0.8f, .b = 0.3f, .a = 1.0f};
constexpr core::Color kSnakeHeadColor{.r = 0.25f, .g = 0.9f, .b = 0.35f, .a = 1.0f};

// Empty space around each snake segment, as a fraction of a cell, so segments are visible.
// The head fills its whole cell, so it looks a bit bigger than the body.
constexpr float kSegmentGap = 0.06f;
constexpr float kHeadGap = 0.0f;

// Where the board sits on screen, in pixels.
struct BoardLayout {
    float cellSize = 0.0f;
    float left = 0.0f;
    float top = 0.0f;
};

} // namespace

namespace { // helpers

// The largest square cells that fit the area, with the board centered in it.
BoardLayout _layout(core::Size area)
{
    const auto width = static_cast<float>(area.width);
    const auto height = static_cast<float>(area.height);
    const float cellSize = std::min(width / kBoardWidth, height / kBoardHeight);

    return {
        .cellSize = cellSize,
        .left = (width - cellSize * kBoardWidth) / 2.0f,
        .top = (height - cellSize * kBoardHeight) / 2.0f,
    };
}

// A cell's rectangle on screen, shrunk by `gap` (fraction of a cell) on every side.
core::Rect _cellRect(const BoardLayout& layout, Cell cell, float gap)
{
    const float inset = layout.cellSize * gap;
    return {
        .x = layout.left + static_cast<float>(cell.x) * layout.cellSize + inset,
        .y = layout.top + static_cast<float>(cell.y) * layout.cellSize + inset,
        .width = layout.cellSize - 2.0f * inset,
        .height = layout.cellSize - 2.0f * inset,
    };
}

// The snake-head shader draws the head facing right; turn it to face `direction`.
// Quarter turns are clockwise on screen (y points down): Right 0, Down 1, Left 2, Up 3.
int _quarterTurns(Direction direction)
{
    switch (direction) {
    case Direction::Right: return 0;
    case Direction::Down:  return 1;
    case Direction::Left:  return 2;
    case Direction::Up:    return 3;
    }
    return 0;
}

} // namespace

std::vector<core::Sprite> gameDrawList(const game::Game& game, core::Size area)
{
    const BoardLayout layout = _layout(area);
    const game::Snake& snake = game.snake();

    std::vector<core::Sprite> sprites;
    sprites.reserve(kBoardWidth * kBoardHeight + snake.body().size());

    for (int y = 0; y < kBoardHeight; ++y) {
        for (int x = 0; x < kBoardWidth; ++x) {
            const bool isEven = (x + y) % 2 == 0;
            sprites.push_back({
                .rect = _cellRect(layout, {x, y}, 0.0f),
                .color = isEven ? kBoardColorA : kBoardColorB,
            });
        }
    }

    // The body (everything but the head), tail first.
    for (const Cell cell : snake.body() | std::views::drop(1) | std::views::reverse) {
        sprites.push_back({.rect = _cellRect(layout, cell, kSegmentGap), .color = kSnakeBodyColor});
    }

    // The head last, so it's drawn on top - with its own shader, facing where the snake goes.
    sprites.push_back({
        .rect = _cellRect(layout, snake.head(), kHeadGap),
        .color = kSnakeHeadColor,
        .shape = core::Shape::SnakeHead,
        .quarterTurns = _quarterTurns(snake.direction()),
    });

    return sprites;
}

} // namespace snake::app
