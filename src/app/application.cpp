#include "snake/app/application.hpp"

#include "snake/app/game_view.hpp"
#include "snake/app/menu_view.hpp"

#include <optional>
#include <random>
#include <vector>

namespace snake::app {

namespace {

// Everything is laid out on a fixed 16:9 canvas of this many pixels. The renderer
// scales it to the window (1:1 at 1920x1080), with black bars if the window or
// monitor isn't 16:9 - so the game looks the same windowed and fullscreen.
constexpr core::Size kCanvas{.width = 1920, .height = 1080};

// The window starts the size of the canvas.
constexpr int kWindowWidth = kCanvas.width;
constexpr int kWindowHeight = kCanvas.height;

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

double _tickSeconds(Speed speed)
{
    return 1.0 / ticksPerSecond(speed);
}

} // namespace

Application::Application()
    : m_window(kWindowWidth, kWindowHeight, "Snake")
    , m_renderer(m_window)
    , m_game(std::random_device{}()) // a different seed (so different food spots) every run
    , m_ticks(_tickSeconds(m_settings.speed), kMaxFrameSeconds)
{
    openMenu();
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
        // In the menu the game is paused: no ticks at all.
        if (m_screen == Screen::Playing) {
            for (int due = m_ticks.advance(m_window.time()); due > 0; --due) {
                m_game.tick();
            }
        }

        std::vector<core::Sprite> sprites = gameDrawList(m_game, kCanvas);
        if (m_screen == Screen::Menu) {
            appendMenu(sprites, m_menu, m_settings, kCanvas);
        }
        m_renderer.drawFrame(sprites, kCanvas);
    }
}

// Each key press, in order. F11 works everywhere; other keys go to whatever is on
// screen: in the game, arrows steer and Escape opens the menu.
void Application::handleInput()
{
    for (const Key key : m_window.takePressedKeys()) {
        if (key == Key::F11) {
            m_settings.fullscreen = !m_settings.fullscreen;
            applySettings();
        } else if (m_screen == Screen::Menu) {
            handleMenuCommand(m_menu.handle(key, m_settings));
        } else if (key == Key::Escape) {
            openMenu();
        } else if (const std::optional<Direction> direction = _toDirection(key)) {
            m_game.turn(*direction);
        }
    }
}

void Application::handleMenuCommand(MenuCommand command)
{
    switch (command) {
    case MenuCommand::None:
        break;
    case MenuCommand::Continue:
        play();
        break;
    case MenuCommand::NewGame:
        m_game.startOver();
        play();
        break;
    case MenuCommand::Quit:
        m_window.close();
        break;
    case MenuCommand::SettingsChanged:
        applySettings();
        break;
    }
}

void Application::openMenu()
{
    m_screen = Screen::Menu;
    m_menu.open(m_hasStarted);
}

void Application::play()
{
    m_screen = Screen::Playing;
    m_hasStarted = true;
    // Time spent in the menu isn't game time: start measuring from now.
    m_ticks.reset(m_window.time());
}

// Make the window and the game's speed match m_settings.
void Application::applySettings()
{
    m_window.setFullscreen(m_settings.fullscreen);
    m_ticks.setStepSeconds(_tickSeconds(m_settings.speed));
}

} // namespace snake::app
