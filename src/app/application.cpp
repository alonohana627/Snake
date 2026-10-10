#include "snake/app/application.hpp"

#include "snake/app/game_view.hpp"

#include <format>
#include <optional>

namespace snake::app {

namespace {

constexpr int kWindowWidth = 1920;
constexpr int kWindowHeight = 1080;

// The longest frame we simulate in full. Anything longer (e.g. while the window is
// being dragged, Windows pauses our loop) counts as this long, so the game doesn't
// race through seconds of catch-up ticks.
constexpr double kMaxFrameSeconds = 0.25;

} // namespace

namespace { // helpers

using game::Direction;
using platform::Key;

// The arrow keys steer the snake. Other keys aren't directions.
std::optional<Direction> _toDirection(Key key)
{
    switch (key) {
    case Key::Up:    return Direction::Up;
    case Key::Down:  return Direction::Down;
    case Key::Left:  return Direction::Left;
    case Key::Right: return Direction::Right;
    default:         return std::nullopt;
    }
}

} // namespace

Application::Application()
    : m_window(kWindowWidth, kWindowHeight, "Snake")
    , m_renderer(m_window)
    , m_ticks(game::kTickSeconds, kMaxFrameSeconds)
{
}

void Application::run()
{
    m_ticks.reset(m_window.time());

    // Main loop, once per frame: input -> as many game ticks as are due -> draw.
    // Presenting waits for the monitor's refresh (vsync), which paces the loop;
    // the game itself advances at its own fixed tick rate.
    while (!m_window.shouldClose()) {
        m_window.pollEvents();

        handleInput();
        if (m_window.shouldClose()) {
            break;
        }

        // Minimized: nothing to draw, so sleep until something happens.
        // Also check for 0x0: while minimizing/restoring there can be a frame where
        // the area is already 0x0 but the window doesn't report "minimized" yet.
        const core::Size area = m_window.framebufferSize();
        if (m_window.isMinimized() || area.width == 0 || area.height == 0) {
            m_window.waitEvents();
            m_ticks.reset(m_window.time()); // time spent minimized isn't game time
            continue;
        }

        // Run the ticks that are due: 0, 1 or several, depending on frame rate vs tick rate.
        for (int due = m_ticks.advance(m_window.time()); due > 0; --due) {
            m_game.tick();
        }

        updateTitle();
        m_renderer.drawFrame(gameDrawList(m_game, area));
    }
}

// There's no text rendering yet, so the score is shown in the window title.
void Application::updateTitle()
{
    if (m_game.score() == m_shownScore && m_game.bestScore() == m_shownBestScore) {
        return;
    }
    m_shownScore = m_game.score();
    m_shownBestScore = m_game.bestScore();
    m_window.setTitle(std::format("Snake  |  Score: {}  |  Best: {}", m_shownScore, m_shownBestScore));
}

// Each key press, in order: Escape quits, arrows ask the snake to turn.
void Application::handleInput()
{
    for (const Key key : m_window.takePressedKeys()) {
        if (key == Key::Escape) {
            m_window.close();
        } else if (const std::optional<Direction> direction = _toDirection(key)) {
            m_game.turn(*direction);
        }
    }
}

} // namespace snake::app
