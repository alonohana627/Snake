#pragma once

namespace snake::app {

// How fast the snake moves. The game itself doesn't know about speed: it moves one
// cell per tick, and the application decides how many ticks there are per second.
enum class Speed {
    Slow,
    Normal,
    Fast,
};

constexpr int ticksPerSecond(Speed speed)
{
    switch (speed) {
    case Speed::Slow:   return 5;
    case Speed::Normal: return 8;
    case Speed::Fast:   return 12;
    }
    return 8;
}

// What the player can change in the Settings menu.
struct Settings {
    bool fullscreen = false;
    Speed speed = Speed::Normal;
};

} // namespace snake::app
