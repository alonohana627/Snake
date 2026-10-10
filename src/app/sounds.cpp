#include "snake/app/sounds.hpp"

#include "snake/audio/synth.hpp"

#include <array>
#include <cstddef>
#include <iterator>
#include <span>
#include <utility>

namespace snake::app {

namespace {

using audio::Note;
using audio::Wave;
using game::EventType;
using game::FoodType;

// Note frequencies, in Hz. Each octave up doubles the frequency (C5 = 2 * C4).
constexpr float kG3 = 196.0f;
constexpr float kC4 = 261.63f;
constexpr float kE4 = 329.63f;
constexpr float kG4 = 392.0f;
constexpr float kC5 = 523.25f;
constexpr float kE5 = 659.25f;
constexpr float kG5 = 783.99f;
constexpr float kA5 = 880.0f;
constexpr float kC6 = 1046.5f;
constexpr float kE6 = 1318.5f;
constexpr float kG6 = 1568.0f;
constexpr float kC7 = 2093.0f;

// The sounds. Mostly notes from the C major chord (C, E, G): any of them sound
// good together, so rising runs sound happy and falling ones sad.
// Each sound's volume is relative to the others; all are kept low - they're
// feedback, not music.

// Start: a rising C major run, "ta-da".
constexpr Note kStart[] = {
    {.wave = Wave::Pulse25, .frequency = kC5, .seconds = 0.07f},
    {.wave = Wave::Pulse25, .frequency = kE5, .seconds = 0.07f},
    {.wave = Wave::Pulse25, .frequency = kG5, .seconds = 0.07f},
    {.wave = Wave::Pulse25, .frequency = kC6, .seconds = 0.2f},
};

// Step: a tiny, low, soft "tuk" - plays 8 times a second, so it must stay in the background.
constexpr Note kStep[] = {
    {.wave = Wave::Triangle, .frequency = kG3, .seconds = 0.03f, .endFrequency = 160.0f},
};

// Eating: a quick upward blip. Bigger food = more notes, higher and brighter.
constexpr Note kEatSmall[] = {
    {.wave = Wave::Pulse25, .frequency = kG5, .seconds = 0.035f},
    {.wave = Wave::Pulse25, .frequency = kC6, .seconds = 0.07f},
};
constexpr Note kEatMedium[] = {
    {.wave = Wave::Pulse25, .frequency = kG5, .seconds = 0.035f},
    {.wave = Wave::Pulse25, .frequency = kC6, .seconds = 0.035f},
    {.wave = Wave::Pulse25, .frequency = kE6, .seconds = 0.08f},
};
constexpr Note kEatLarge[] = {
    {.wave = Wave::Pulse12, .frequency = kG5, .seconds = 0.03f},
    {.wave = Wave::Pulse12, .frequency = kC6, .seconds = 0.03f},
    {.wave = Wave::Pulse12, .frequency = kE6, .seconds = 0.03f},
    {.wave = Wave::Pulse12, .frequency = kG6, .seconds = 0.03f},
    {.wave = Wave::Pulse12, .frequency = kC7, .seconds = 0.12f},
};

// Crash: a short crunch of noise, then a falling "wah-wah-waaah" that slides down at the end.
constexpr Note kCrash[] = {
    {.wave = Wave::Noise, .frequency = 1200.0f, .seconds = 0.09f, .endFrequency = 200.0f},
    {.wave = Wave::Pulse50, .frequency = kG4, .seconds = 0.12f},
    {.wave = Wave::Pulse50, .frequency = kE4, .seconds = 0.12f},
    {.wave = Wave::Pulse50, .frequency = kC4, .seconds = 0.35f, .endFrequency = kG3},
};

// Menu: short and thin, so scrolling through items doesn't get tiring.
constexpr Note kMenuMove[] = {
    {.wave = Wave::Pulse12, .frequency = kA5, .seconds = 0.03f},
};
constexpr Note kMenuSelect[] = {
    {.wave = Wave::Pulse25, .frequency = kE5, .seconds = 0.04f},
    {.wave = Wave::Pulse25, .frequency = kA5, .seconds = 0.08f},
};
constexpr Note kMenuBack[] = {
    {.wave = Wave::Pulse25, .frequency = kA5, .seconds = 0.04f},
    {.wave = Wave::Pulse25, .frequency = kE5, .seconds = 0.08f},
};

struct SoundDesign {
    std::span<const Note> notes;
    float volume = 1.0f;
};

// In Sfx order.
constexpr SoundDesign kDesigns[] = {
    {kStart, 0.2f},
    {kStep, 0.12f},
    {kEatSmall, 0.2f},
    {kEatMedium, 0.2f},
    {kEatLarge, 0.18f},
    {kCrash, 0.22f},
    {kMenuMove, 0.12f},
    {kMenuSelect, 0.16f},
    {kMenuBack, 0.16f},
};
static_assert(std::size(kDesigns) == kSfxCount, "one design per Sfx");

// The second step sound is this much lower (about two semitones).
constexpr float kStepDownPitch = 0.89f;

} // namespace

namespace { // helpers

Sfx _eatSound(FoodType food)
{
    switch (food) {
    case FoodType::Small:  return Sfx::EatSmall;
    case FoodType::Medium: return Sfx::EatMedium;
    case FoodType::Large:  return Sfx::EatLarge;
    }
    return Sfx::EatSmall;
}

} // namespace

std::vector<float> renderSfx(Sfx sfx, std::uint32_t sampleRate)
{
    const SoundDesign& design = kDesigns[std::to_underlying(sfx)];
    return audio::render(design.notes, design.volume, sampleRate);
}

Sounds::Sounds(audio::Player& player)
    : m_player(player)
{
    for (std::size_t i = 0; i < kSfxCount; ++i) {
        m_sounds[i] = m_player.add(renderSfx(static_cast<Sfx>(i), m_player.sampleRate()));
    }
}

void Sounds::play(Sfx sfx)
{
    m_player.play(m_sounds[std::to_underlying(sfx)]);
}

void Sounds::playGameEvents(std::span<const game::Event> events)
{
    // Collect first, then play each at most once.
    std::array<bool, kSfxCount> due{};
    for (const game::Event& event : events) {
        switch (event.type) {
        case EventType::Moved:   due[std::to_underlying(Sfx::Step)] = true; break;
        case EventType::Ate:     due[std::to_underlying(_eatSound(event.food))] = true; break;
        case EventType::Crashed: due[std::to_underlying(Sfx::Crash)] = true; break;
        }
    }

    // A crash says it all: no step or eating sound on top of it.
    if (due[std::to_underlying(Sfx::Crash)]) {
        play(Sfx::Crash);
        return;
    }
    for (std::size_t i = 0; i < kSfxCount; ++i) {
        if (!due[i]) {
            continue;
        }
        if (static_cast<Sfx>(i) == Sfx::Step) {
            m_stepUp = !m_stepUp;
            m_player.play(m_sounds[i], 1.0f, m_stepUp ? 1.0f : kStepDownPitch);
        } else {
            m_player.play(m_sounds[i]);
        }
    }
}

} // namespace snake::app
