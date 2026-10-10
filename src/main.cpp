#include "snake/core/fixed_timestep.hpp"
#include "snake/game/game.hpp"
#include "snake/gfx/renderer.hpp"
#include "snake/platform/window.hpp"

#include <cstdio>
#include <exception>
#include <print>

namespace {

constexpr int kWindowWidth = 800;
constexpr int kWindowHeight = 800;

// The longest frame we simulate in full. Anything longer (e.g. while the window is
// being dragged, Windows pauses our loop) counts as this long, so the game doesn't
// race through seconds of catch-up ticks.
constexpr double kMaxFrameSeconds = 0.25;

} // namespace

namespace { // helpers

using snake::platform::Key;
using snake::platform::Window;

// -1, 0 or +1, depending on which of the two keys is held down right now.
float _axis(const Window& window, Key negative, Key positive)
{
    float value = 0.0f;
    if (window.isKeyDown(negative)) {
        value -= 1.0f;
    }
    if (window.isKeyDown(positive)) {
        value += 1.0f;
    }
    return value;
}

// Turn the keyboard state into what the game understands.
snake::game::Input _readInput(const Window& window)
{
    return {
        .moveX = _axis(window, Key::Left, Key::Right),
        .moveY = _axis(window, Key::Up, Key::Down),
    };
}

} // namespace

int main()
{
    // Print immediately, even when stdout is a pipe (some terminals) rather than a console.
    std::setvbuf(stdout, nullptr, _IONBF, 0);

    try {
        // Declared in this order, so destroyed in reverse: the renderer goes
        // before the window it draws into.
        Window window(kWindowWidth, kWindowHeight, "Snake");
        snake::gfx::Renderer renderer(window);
        snake::game::Game game(window.framebufferSize());

        snake::core::FixedTimestep ticks(snake::game::kTickSeconds, kMaxFrameSeconds);
        ticks.reset(window.time());

        // Main loop, once per frame: input -> as many game ticks as are due -> draw.
        // Presenting waits for the monitor's refresh (vsync), which paces the loop;
        // the game itself advances at its own fixed tick rate.
        while (!window.shouldClose()) {
            window.pollEvents();

            if (window.isKeyDown(Key::Escape)) {
                window.close();
                continue;
            }

            // Minimized: nothing to draw, so sleep until something happens.
            // Also check for 0x0: while minimizing/restoring there can be a frame where
            // the area is already 0x0 but the window doesn't report "minimized" yet.
            const snake::core::Size area = window.framebufferSize();
            if (window.isMinimized() || area.width == 0 || area.height == 0) {
                window.waitEvents();
                ticks.reset(window.time()); // time spent minimized isn't game time
                continue;
            }

            // Run the ticks that are due: 0, 1 or several, depending on frame rate vs tick rate.
            const snake::game::Input input = _readInput(window);
            for (int due = ticks.advance(window.time()); due > 0; --due) {
                game.tick(input, area);
            }

            renderer.drawFrame(game.drawList());
        }
    } catch (const std::exception& error) {
        std::println(stderr, "Error: {}", error.what());
        return 1;
    }

    return 0;
}
