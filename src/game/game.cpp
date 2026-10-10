#include "snake/game/game.hpp"

#include <algorithm>

namespace snake::game {

namespace {

constexpr float kPlayerSize = 50.0f;   // pixels
constexpr float kPlayerSpeed = 450.0f; // pixels per second

// Every tick is the same length, so the distance per tick is a constant: 450 / 60 = 7.5 px.
constexpr float kPlayerStep = kPlayerSpeed / kTicksPerSecond;

constexpr core::Color kPlayerColor{.r = 0.2f, .g = 0.8f, .b = 0.3f, .a = 1.0f};

} // namespace

Game::Game(core::Size area)
    : m_player{
          .x = (static_cast<float>(area.width) - kPlayerSize) / 2.0f,
          .y = (static_cast<float>(area.height) - kPlayerSize) / 2.0f,
          .width = kPlayerSize,
          .height = kPlayerSize,
      }
{
}

void Game::tick(const Input& input, core::Size area)
{
    m_player.x += input.moveX * kPlayerStep;
    m_player.y += input.moveY * kPlayerStep;

    // Keep it inside the play area.
    const float maxX = std::max(0.0f, static_cast<float>(area.width) - m_player.width);
    const float maxY = std::max(0.0f, static_cast<float>(area.height) - m_player.height);
    m_player.x = std::clamp(m_player.x, 0.0f, maxX);
    m_player.y = std::clamp(m_player.y, 0.0f, maxY);
}

std::vector<core::ColoredRect> Game::drawList() const
{
    return {{.rect = m_player, .color = kPlayerColor}};
}

} // namespace snake::game
