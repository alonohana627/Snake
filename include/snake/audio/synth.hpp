#pragma once

#include <cstdint>
#include <span>
#include <vector>

namespace snake::audio {

// The sounds an 8-bit console's sound chip can make. Each one is a simple shape
// repeated `frequency` times per second; the shape decides the "color" of the sound.
enum class Wave {
    Pulse50,  // square wave: on half the time. Hollow, like a clarinet.
    Pulse25,  // on a quarter of the time: brighter, the classic NES lead.
    Pulse12,  // on an eighth: thin and buzzy.
    Triangle, // soft and round, in 16 steps like the NES (its bass channel).
    Noise,    // random on/off: hiss, crunch, explosions. Frequency = how fast it changes.
};

// One note: a wave played for `seconds`, sliding from `frequency` to
// `endFrequency` (0 = no slide). Each note starts loud and fades (a "blip").
struct Note {
    Wave wave = Wave::Pulse25;
    float frequency = 440.0f; // Hz
    float seconds = 0.1f;
    float endFrequency = 0.0f;
    float volume = 1.0f;
};

// Turns notes, played one after another, into sound samples: one float per
// 1/sampleRate seconds, between -1 and 1 (mono). `volume` scales the whole sound.
std::vector<float> render(std::span<const Note> notes, float volume, std::uint32_t sampleRate);

} // namespace snake::audio
