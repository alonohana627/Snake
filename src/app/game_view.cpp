#include "snake/app/game_view.hpp"

#include "snake/app/text.hpp"

#include <algorithm>
#include <format>
#include <iterator>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>

namespace snake::app {

namespace {

using game::Cell;
using game::Direction;
using game::FoodType;
using game::kBoardHeight;
using game::kBoardWidth;

// All colors are linear ("how much light"); the screen shows them brighter (sRGB).

// Behind everything: a dark teal, fading darker toward the window's edges.
constexpr core::Color kBackgroundColor{.r = 0.035f, .g = 0.06f, .b = 0.07f, .a = 1.0f};

// The board: a dark grass checkerboard inside a mossy frame, so the bright snake
// and fruit stand out on it.
constexpr core::Color kFrameColor{.r = 0.06f, .g = 0.11f, .b = 0.07f, .a = 1.0f};
// With solid walls the frame becomes a wall: cool grey stone, clearly lighter than the
// mossy frame and a hue no actor uses (the snake is green, the fruit warm), so
// "this edge is dangerous" reads at a glance - also without color vision, by value.
constexpr core::Color kWallColor{.r = 0.17f, .g = 0.18f, .b = 0.21f, .a = 1.0f};
constexpr core::Color kBoardColorA{.r = 0.03f, .g = 0.075f, .b = 0.035f, .a = 1.0f};
constexpr core::Color kBoardColorB{.r = 0.04f, .g = 0.095f, .b = 0.045f, .a = 1.0f};

constexpr core::Color kSnakeBodyColor{.r = 0.2f, .g = 0.8f, .b = 0.3f, .a = 1.0f};
constexpr core::Color kSnakeHeadColor{.r = 0.25f, .g = 0.9f, .b = 0.35f, .a = 1.0f};

// Space around the board for the frame, in cells on each side, and how much of
// it the frame covers. The rest is breathing room up to the window's edge.
constexpr float kBoardMargin = 0.75f;
constexpr float kFrameWidth = 0.3f;
constexpr float kWallWidth = 0.45f; // solid walls are also thicker: a shape cue, not just color

// Side panel text: muted labels, bright numbers.
constexpr core::Color kLabelColor{.r = 0.3f, .g = 0.5f, .b = 0.32f, .a = 1.0f};
constexpr core::Color kValueColor{.r = 0.9f, .g = 0.95f, .b = 0.85f, .a = 1.0f};
constexpr core::Color kHintColor{.r = 0.2f, .g = 0.32f, .b = 0.22f, .a = 1.0f};

// Font pixel sizes (a character is 5x7 of these).
constexpr float kLabelPixels = 5.0f;
constexpr float kScorePixels = 14.0f;
constexpr float kBestPixels = 8.0f;
constexpr float kHintPixels = 3.0f;

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

// How each type of food looks: its color, and how much space it leaves in its
// cell (so bigger food really looks bigger).
struct FoodLook {
    core::Color color;
    float gap = 0.0f;
};

} // namespace

namespace { // helpers

// The largest square cells that fit the area (with the margin around the board),
// with the board centered in it.
BoardLayout _layout(core::Size area)
{
    const auto width = static_cast<float>(area.width);
    const auto height = static_cast<float>(area.height);
    const float cellSize = std::min(width / (kBoardWidth + 2.0f * kBoardMargin),
                                    height / (kBoardHeight + 2.0f * kBoardMargin));

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

// The whole board, grown by `border` (in cells) on every side.
core::Rect _boardRect(const BoardLayout& layout, float border)
{
    const float grow = layout.cellSize * border;
    return {
        .x = layout.left - grow,
        .y = layout.top - grow,
        .width = layout.cellSize * kBoardWidth + 2.0f * grow,
        .height = layout.cellSize * kBoardHeight + 2.0f * grow,
    };
}

// Small = red apple, Medium = orange, Large = big golden apple.
FoodLook _foodLook(FoodType type)
{
    switch (type) {
    case FoodType::Small:  return {.color = {.r = 0.85f, .g = 0.08f, .b = 0.06f, .a = 1.0f}, .gap = 0.14f};
    case FoodType::Medium: return {.color = {.r = 1.0f, .g = 0.35f, .b = 0.02f, .a = 1.0f}, .gap = 0.06f};
    case FoodType::Large:  return {.color = {.r = 1.0f, .g = 0.75f, .b = 0.08f, .a = 1.0f}, .gap = -0.08f};
    }
    return {.color = {.r = 0.85f, .g = 0.08f, .b = 0.06f, .a = 1.0f}, .gap = 0.14f};
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

// Left of the board: the score, big, and the best score under it.
void _appendScorePanel(std::vector<core::Sprite>& sprites, const game::Game& game, const BoardLayout& layout)
{
    const float centerX = _boardRect(layout, kBoardMargin).x / 2.0f; // middle of the space left of the board
    float y = layout.top + layout.cellSize;

    appendText(sprites, "SCORE", centerX, y, {.pixelSize = kLabelPixels, .color = kLabelColor, .align = Align::Center});
    y += textHeight(kLabelPixels) + 20.0f;
    appendText(sprites, std::to_string(game.score()), centerX, y,
               {.pixelSize = kScorePixels, .color = kValueColor, .align = Align::Center, .shadow = true});
    y += textHeight(kScorePixels) + 60.0f;

    appendText(sprites, "BEST", centerX, y, {.pixelSize = kLabelPixels, .color = kLabelColor, .align = Align::Center});
    y += textHeight(kLabelPixels) + 16.0f;
    appendText(sprites, std::to_string(game.bestScore()), centerX, y,
               {.pixelSize = kBestPixels, .color = kValueColor, .align = Align::Center, .shadow = true});
}

// Right of the board: what each food is worth, and the controls at the bottom.
void _appendInfoPanel(std::vector<core::Sprite>& sprites, const BoardLayout& layout, core::Size canvas)
{
    const core::Rect board = _boardRect(layout, kBoardMargin);
    const float panelLeft = board.x + board.width;
    const float centerX = (panelLeft + static_cast<float>(canvas.width)) / 2.0f;
    float y = layout.top + layout.cellSize;

    appendText(sprites, "FOOD", centerX, y, {.pixelSize = kLabelPixels, .color = kLabelColor, .align = Align::Center});
    y += textHeight(kLabelPixels) + 30.0f;

    // One row per type: the fruit as on the board, then "+points".
    const float iconSize = layout.cellSize;
    const float rowWidth = iconSize + 24.0f + textWidth("+3", kLabelPixels);
    const float rowLeft = centerX - rowWidth / 2.0f;
    for (const FoodType type : {FoodType::Small, FoodType::Medium, FoodType::Large}) {
        const FoodLook look = _foodLook(type);
        const float inset = iconSize * look.gap;
        sprites.push_back({
            .rect = {.x = rowLeft + inset, .y = y + inset, .width = iconSize - 2.0f * inset, .height = iconSize - 2.0f * inset},
            .color = look.color,
            .shape = core::Shape::Food,
        });
        appendText(sprites, std::format("+{}", game::pointsOf(type)), rowLeft + iconSize + 24.0f,
                   y + (iconSize - textHeight(kLabelPixels)) / 2.0f,
                   {.pixelSize = kLabelPixels, .color = kValueColor, .shadow = true});
        y += iconSize + 20.0f;
    }

    // Controls, along the bottom.
    constexpr std::string_view kHints[] = {"ARROWS: MOVE", "ESC: MENU", "F11: FULLSCREEN"};
    const float lineHeight = textHeight(kHintPixels) + 14.0f;
    y = layout.top + layout.cellSize * kBoardHeight - lineHeight * static_cast<float>(std::size(kHints));
    for (const std::string_view hint : kHints) {
        appendText(sprites, hint, centerX, y, {.pixelSize = kHintPixels, .color = kHintColor, .align = Align::Center});
        y += lineHeight;
    }
}

} // namespace

std::vector<core::Sprite> gameDrawList(const game::Game& game, core::Size canvas)
{
    const BoardLayout layout = _layout(canvas);
    const game::Snake& snake = game.snake();

    std::vector<core::Sprite> sprites;
    sprites.reserve(2 + kBoardWidth * kBoardHeight + 1 + snake.body().size() + 100); // + ~100 for the side panels

    // The window's background, then the board's frame.
    sprites.push_back({
        .rect = {.x = 0.0f, .y = 0.0f, .width = static_cast<float>(canvas.width), .height = static_cast<float>(canvas.height)},
        .color = kBackgroundColor,
        .shape = core::Shape::Background,
    });
    const bool solidWalls = game.walls() == game::Walls::Solid;
    sprites.push_back({
        .rect = _boardRect(layout, solidWalls ? kWallWidth : kFrameWidth),
        .color = solidWalls ? kWallColor : kFrameColor,
    });

    for (int y = 0; y < kBoardHeight; ++y) {
        for (int x = 0; x < kBoardWidth; ++x) {
            const bool isEven = (x + y) % 2 == 0;
            sprites.push_back({
                .rect = _cellRect(layout, {x, y}, 0.0f),
                .color = isEven ? kBoardColorA : kBoardColorB,
            });
        }
    }

    // A negative gap lets big food spill a little over its cell's edges.
    if (const std::optional<game::Food> food = game.food()) {
        const FoodLook look = _foodLook(food->type);
        sprites.push_back({
            .rect = _cellRect(layout, food->cell, look.gap),
            .color = look.color,
            .shape = core::Shape::Food,
        });
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

    _appendScorePanel(sprites, game, layout);
    _appendInfoPanel(sprites, layout, canvas);

    return sprites;
}

} // namespace snake::app
