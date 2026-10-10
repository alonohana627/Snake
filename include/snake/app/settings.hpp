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

// How loud the sound effects are.
enum class Volume {
    Off,
    Low,
    Medium,
    High,
};

// Loudness isn't linear: each level is about half as loud as the next (-6 dB per step),
// which sounds like even steps. Linear steps (0.33, 0.66, 1) would sound uneven: the
// top two would barely differ.
constexpr float amplitudeOf(Volume volume)
{
    switch (volume) {
    case Volume::Off:    return 0.0f;
    case Volume::Low:    return 0.25f;
    case Volume::Medium: return 0.5f;
    case Volume::High:   return 1.0f;
    }
    return 0.5f;
}

// What the player can change in the Settings menu.
struct Settings {
    bool fullscreen = false;
    Speed speed = Speed::Normal;
    Volume volume = Volume::Medium;
};

} // namespace snake::app
