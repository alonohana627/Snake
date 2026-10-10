#include "snake/audio/player.hpp"

#include <miniaudio.h>

#include <array>
#include <print>
#include <utility>

namespace snake::audio {

namespace {

// How many copies of one sound can play at the same time.
constexpr std::size_t kVoicesPerSound = 4;

// Used when there's no audio device, so sounds can still be rendered and added.
constexpr std::uint32_t kFallbackSampleRate = 48000;

// One playable copy of a sound: a miniaudio "data source" that reads our samples
// (ma_audio_buffer, with its own read position) and a ma_sound that plays it.
// miniaudio keeps pointers to both, so a Voice must never move: we heap-allocate it.
struct Voice {
    ma_audio_buffer buffer{};
    ma_sound sound{};
};

struct Sound {
    std::vector<float> samples; // the voices read from here; never resized after add()
    std::array<std::unique_ptr<Voice>, kVoicesPerSound> voices;
    std::size_t nextVoice = 0; // the one that started longest ago
};

} // namespace

struct Player::Impl {
    ma_engine engine{};
    bool available = false;
    std::vector<std::unique_ptr<Sound>> sounds;

    // Sounds before the engine: they're attached to it.
    ~Impl()
    {
        if (!available) {
            return;
        }
        for (const std::unique_ptr<Sound>& sound : sounds) {
            for (const std::unique_ptr<Voice>& voice : sound->voices) {
                ma_sound_uninit(&voice->sound);
                ma_audio_buffer_uninit(&voice->buffer);
            }
        }
        ma_engine_uninit(&engine);
    }
};

Player::Player()
    : m_impl(std::make_unique<Impl>())
{
    // The engine opens the default output device and starts mixing on its own thread.
    if (ma_engine_init(nullptr, &m_impl->engine) != MA_SUCCESS) {
        std::println(stderr, "Audio: no output device, the game will be silent");
        return;
    }
    m_impl->available = true;
}

Player::~Player() = default;

bool Player::isAvailable() const
{
    return m_impl->available;
}

std::uint32_t Player::sampleRate() const
{
    return m_impl->available ? ma_engine_get_sample_rate(&m_impl->engine) : kFallbackSampleRate;
}

SoundId Player::add(std::vector<float> samples)
{
    const SoundId id{m_impl->sounds.size()};
    auto& sound = m_impl->sounds.emplace_back(std::make_unique<Sound>());
    sound->samples = std::move(samples);
    if (!m_impl->available) {
        return id;
    }

    for (std::unique_ptr<Voice>& voice : sound->voices) {
        voice = std::make_unique<Voice>();

        // The buffer reads straight from sound->samples (no copy): 32-bit float, 1 channel.
        ma_audio_buffer_config config = ma_audio_buffer_config_init(
            ma_format_f32, 1, sound->samples.size(), sound->samples.data(), nullptr);
        config.sampleRate = sampleRate();
        ma_audio_buffer_init(&config, &voice->buffer);

        // NO_SPATIALIZATION: no 3D positioning, it's a 2D game.
        ma_sound_init_from_data_source(&m_impl->engine, &voice->buffer, MA_SOUND_FLAG_NO_SPATIALIZATION,
                                       nullptr, &voice->sound);
    }
    return id;
}

void Player::play(SoundId id, float volume, float pitch)
{
    if (!m_impl->available || id.index >= m_impl->sounds.size()) {
        return;
    }
    Sound& sound = *m_impl->sounds[id.index];

    // Take turns between the voices: the next one is the one that started longest
    // ago, so if all are busy, the oldest is cut off and restarted.
    ma_sound* const voice = &sound.voices[sound.nextVoice]->sound;
    sound.nextVoice = (sound.nextVoice + 1) % kVoicesPerSound;

    ma_sound_seek_to_pcm_frame(voice, 0);
    ma_sound_set_volume(voice, volume);
    ma_sound_set_pitch(voice, pitch);
    ma_sound_start(voice);
}

void Player::setMasterVolume(float volume)
{
    if (m_impl->available) {
        ma_engine_set_volume(&m_impl->engine, volume);
    }
}

} // namespace snake::audio
