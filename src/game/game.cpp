#include "snake/game/game.hpp"

#include <algorithm>

namespace snake::game {

namespace {

constexpr Cell kStartHead{kBoardWidth / 2, kBoardHeight / 2};
constexpr int kStartLength = 5;
constexpr Direction kStartDirection = Direction::Right;

} // namespace

namespace { // helpers

Snake _startingSnake()
{
    return Snake(kStartHead, kStartLength, kStartDirection);
}

} // namespace

Game::Game()
    : m_snake(_startingSnake())
{
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

    ++m_ticksSurvived;
    m_bestScore = std::max(m_bestScore, score());
}

// Back to the start: a fresh snake (no queued turns either) and a zero score.
// The best score is kept.
void Game::startOver()
{
    m_snake = _startingSnake();
    m_ticksSurvived = 0;
}

} // namespace snake::game
