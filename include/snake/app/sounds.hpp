#pragma once

#include "snake/audio/player.hpp"
#include "snake/game/game.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace snake::app {

// Every sound effect in the game.
enum class Sfx : std::size_t { // size_t: the values index arrays
    Start,      // a new game begins
    Step,       // the snake moves one cell
    EatSmall,
    EatMedium,
    EatLarge,
    Crash,      // the snake ran into itself: game over, start over
    MenuMove,   // selection moved, or a setting changed
    MenuSelect, // into a page, or back to the game
    MenuBack,   // out of a page, or into the menu
};
inline constexpr std::size_t kSfxCount = 9;

// The samples of one sound effect (mono, -1..1) at `sampleRate`. Sounds uses this;
// it's also handy on its own, e.g. to write a sound to a .wav file and listen to it.
std::vector<float> renderSfx(Sfx sfx, std::uint32_t sampleRate);

// How the game sounds: designs every effect as 8-bit notes, renders them once at
// startup, and picks which to play. The audio counterpart of game_view.
class Sounds {
public:
    explicit Sounds(audio::Player& player);

    void play(Sfx sfx);

    // The sounds for what happened in the game since last time. If several ticks
    // ran in one frame, each sound still plays once (not 3 steps on top of each other).
    void playGameEvents(std::span<const game::Event> events);

private:
    audio::Player& m_player;
    std::array<audio::SoundId, kSfxCount> m_sounds;
    bool m_stepUp = false; // steps alternate between two pitches: "tick, tock"
};

} // namespace snake::app
