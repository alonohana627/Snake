#include "snake/audio/synth.hpp"

#include <algorithm>
#include <cmath>
#include <numbers>

namespace snake::audio {

namespace {

// Every note fades in over this long and out over the last kReleaseSeconds.
// Jumping straight from silence to full volume (or back) makes an audible click.
constexpr float kAttackSeconds = 0.002f;
constexpr float kReleaseSeconds = 0.006f;

// How fast a note fades after its start: volume = e^(-kDecay * progress), so
// it's at ~5% by its end. That quick fade is what makes it a "blip", not a beep.
constexpr float kDecay = 3.0f;

// A gentle low-pass filter (cuts frequencies above this) takes the harsh edge
// off the square waves, so they sound soft rather than piercing.
constexpr float kLowPassHz = 5000.0f;

} // namespace

namespace { // helpers

// The wave's value at `phase` (0..1 = one repetition).
// `noise` is the current noise output (the noise wave doesn't depend on phase).
float _waveAt(Wave wave, float phase, float noise)
{
    switch (wave) {
    case Wave::Pulse50: return phase < 0.5f ? 1.0f : -1.0f;
    case Wave::Pulse25: return phase < 0.25f ? 1.0f : -1.0f;
    case Wave::Pulse12: return phase < 0.125f ? 1.0f : -1.0f;
    case Wave::Triangle: {
        const float smooth = 4.0f * std::abs(phase - 0.5f) - 1.0f; // -1..1..-1
        return std::round(smooth * 7.5f) / 7.5f;                  // 16 steps, like the NES
    }
    case Wave::Noise: return noise;
    }
    return 0.0f;
}

// The NES noise generator: a 15-bit shift register. Each step shifts it right and
// feeds back bit 0 XOR bit 1 into the top - a fixed, random-sounding pattern.
struct NoiseGenerator {
    std::uint16_t bits = 1;

    float step()
    {
        const auto feedback = static_cast<std::uint16_t>((bits ^ (bits >> 1)) & 1u);
        bits = static_cast<std::uint16_t>((bits >> 1) | (feedback << 14));
        return (bits & 1u) != 0 ? 1.0f : -1.0f;
    }
};

// How loud the note is at `time` seconds into it (0..1).
float _envelope(float time, float seconds)
{
    const float attack = std::min(time / kAttackSeconds, 1.0f);
    const float release = std::clamp((seconds - time) / kReleaseSeconds, 0.0f, 1.0f);
    const float decay = std::exp(-kDecay * time / seconds);
    return attack * decay * release;
}

} // namespace

std::vector<float> render(std::span<const Note> notes, float volume, std::uint32_t sampleRate)
{
    const auto rate = static_cast<float>(sampleRate);
    std::vector<float> samples;

    float phase = 0.0f; // where we are in the wave's repetition, 0..1
    NoiseGenerator noise;
    float noiseValue = noise.step();

    for (const Note& note : notes) {
        const auto count = static_cast<std::size_t>(note.seconds * rate);
        const float endFrequency = note.endFrequency > 0.0f ? note.endFrequency : note.frequency;

        for (std::size_t i = 0; i < count; ++i) {
            const float time = static_cast<float>(i) / rate;
            const float progress = time / note.seconds;

            // Slide in equal musical steps (multiplying), not equal Hz: it sounds even.
            const float frequency = note.frequency * std::pow(endFrequency / note.frequency, progress);

            samples.push_back(_waveAt(note.wave, phase, noiseValue) * _envelope(time, note.seconds) * note.volume * volume);

            // Advance the wave. Each time a repetition ends, noise picks its next value.
            phase += frequency / rate;
            if (phase >= 1.0f) {
                phase -= std::floor(phase);
                noiseValue = noise.step();
            }
        }
    }

    // One-pole low-pass filter: each output moves part of the way toward the input,
    // which smooths out the sharpest (highest-frequency) edges.
    const float smoothing = 1.0f - std::exp(-2.0f * std::numbers::pi_v<float> * kLowPassHz / rate);
    float filtered = 0.0f;
    for (float& sample : samples) {
        filtered += smoothing * (sample - filtered);
        sample = filtered;
    }
    return samples;
}

} // namespace snake::audio
