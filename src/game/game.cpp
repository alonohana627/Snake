#include "snake/game/game.hpp"

#include <algorithm>
#include <cstdlib>
#include <iterator>
#include <vector>

namespace snake::game {

namespace {

constexpr Cell kStartHead{kBoardWidth / 2, kBoardHeight / 2};
constexpr int kStartLength = 5;
constexpr Direction kStartDirection = Direction::Right;

// New food appears at least this many cells (moves) away from the head when
// possible, so it never pops up right where the snake would eat it for free.
constexpr int kFoodMinDistance = 3;

// How often each food type appears, relative to each other, in FoodType order:
// Small 6 : Medium 3 : Large 1, i.e. 60% / 30% / 10%.
constexpr double kFoodWeights[] = {6.0, 3.0, 1.0};

} // namespace

namespace { // helpers

Snake _startingSnake()
{
    return Snake(kStartHead, kStartLength, kStartDirection);
}

// How many moves it takes to get from a to b, counting the wrap-around edges
// (e.g. on a 20-wide board, x = 0 and x = 19 are 1 move apart).
int _distance(Cell a, Cell b)
{
    const int dx = std::abs(a.x - b.x);
    const int dy = std::abs(a.y - b.y);
    return std::min(dx, kBoardWidth - dx) + std::min(dy, kBoardHeight - dy);
}

} // namespace

Game::Game(std::uint32_t seed)
    : m_random(seed)
    , m_snake(_startingSnake())
{
    spawnFood();
}

void Game::turn(Direction direction)
{
    m_snake.turn(direction);
}

void Game::tick()
{
    // No walls yet: leaving one side of the board brings the snake back on the other.
    m_snake.stepTo(wrap(m_snake.nextHead(), kBoardWidth, kBoardHeight));

    // The snake must not run into itself.
    if (m_snake.hasBittenItself()) {
        startOver();
        return;
    }

    // Eating: longer by the food's growth (the tail stays put for that many steps),
    // its points added to the score, and new food.
    if (m_food && m_snake.head() == m_food->cell) {
        m_snake.grow(growthOf(m_food->type));
        m_score += pointsOf(m_food->type);
        m_bestScore = std::max(m_bestScore, m_score);
        spawnFood();
    }
}

// Back to the start: a fresh snake (no queued turns either), a zero score and
// new food. The best score is kept.
void Game::startOver()
{
    m_snake = _startingSnake();
    m_score = 0;
    spawnFood();
}

// Put new food on a random free cell, every free cell equally likely, and pick
// its type by kFoodWeights. Prefer cells not too close to the head; if there are
// none, any free cell will do.
void Game::spawnFood()
{
    std::vector<Cell> freeCells;
    std::vector<Cell> farCells; // free and at least kFoodMinDistance from the head
    for (int y = 0; y < kBoardHeight; ++y) {
        for (int x = 0; x < kBoardWidth; ++x) {
            const Cell cell{x, y};
            if (m_snake.occupies(cell)) {
                continue;
            }
            freeCells.push_back(cell);
            if (_distance(cell, m_snake.head()) >= kFoodMinDistance) {
                farCells.push_back(cell);
            }
        }
    }

    const std::vector<Cell>& candidates = farCells.empty() ? freeCells : farCells;
    if (candidates.empty()) {
        m_food.reset(); // the snake fills the whole board
        return;
    }

    std::uniform_int_distribution<std::size_t> pickCell(0, candidates.size() - 1);
    std::discrete_distribution<int> pickType(std::begin(kFoodWeights), std::end(kFoodWeights));
    m_food = Food{
        .cell = candidates[pickCell(m_random)],
        .type = static_cast<FoodType>(pickType(m_random)),
    };
}

} // namespace snake::game
