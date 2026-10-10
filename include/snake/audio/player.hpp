#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

namespace snake::audio {

// Which sound to play: returned by Player::add.
struct SoundId {
    std::size_t index = 0;
};

// Plays short sounds through the computer's default audio output (via miniaudio).
//
// miniaudio mixes on its own thread, which asks for more samples every few
// milliseconds; this class only tells it what to play, from the main thread.
//
// No audio device (or it fails to open)? Everything still works - silently.
class Player {
public:
    Player();
    ~Player();

    Player(const Player&) = delete;
    Player& operator=(const Player&) = delete;

    bool isAvailable() const;

    // Samples per second the output runs at. Render sounds at this rate.
    std::uint32_t sampleRate() const;

    // Keeps a sound (mono samples, -1..1, at sampleRate()) ready to play.
    // Do this up front: adding allocates, playing doesn't.
    SoundId add(std::vector<float> samples);

    // Starts the sound. The same sound can overlap itself a few times; past that,
    // its oldest copy restarts. pitch: 1 = as rendered, 2 = an octave up (and twice as fast).
    void play(SoundId sound, float volume = 1.0f, float pitch = 1.0f);

    // Volume of everything, 0..1.
    void setMasterVolume(float volume);

private:
    // All the miniaudio parts live in player.cpp, so including this header doesn't
    // pull in miniaudio (the same idea as GLFW staying inside platform).
    struct Impl;
    std::unique_ptr<Impl> m_impl;
};

} // namespace snake::audio
