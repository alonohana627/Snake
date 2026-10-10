# Audio (`audio` + `app/sounds`)

All sounds are **generated in code** at startup, the way an 8-bit console's sound
chip makes them: square, triangle and noise waves. There are no audio files.

```
app/sounds.cpp      what each sound is (notes) and when it plays
      │  render(notes) at startup
audio/synth.cpp     notes -> samples (float numbers, -1..1)
      │  add(samples), play(id)
audio/player.cpp    samples -> speakers, through miniaudio
```

## Sound as numbers

A sound is a list of **samples**: how far the speaker is pushed, measured thousands
of times a second (the **sample rate**, usually 48000). A wave repeating 440 times a
second is the note A4. Louder = bigger numbers. Everything below is about
producing that list of numbers.

## The synth (`audio/synth.cpp`)

A sound is a sequence of **notes** played one after another
([synth.hpp](../include/snake/audio/synth.hpp)):

```cpp
struct Note {
    Wave wave;            // Pulse50, Pulse25, Pulse12, Triangle, Noise
    float frequency;      // Hz
    float seconds;
    float endFrequency;   // slide to this by the end (0 = no slide)
    float volume;
};
std::vector<float> render(std::span<const Note>, float volume, std::uint32_t sampleRate);
```

| Wave | Shape | Sounds like |
|---|---|---|
| `Pulse50` | on half the time, off half | hollow (square wave) |
| `Pulse25` | on a quarter of the time | brighter - the classic NES lead |
| `Pulse12` | on an eighth | thin, buzzy |
| `Triangle` | up and down in 16 steps (like the NES) | soft, round |
| `Noise` | a 15-bit shift register (the NES noise channel) | hiss, crunch |

For each sample, `render`:

1. Reads the wave at the current **phase** (0..1 = one repetition), then advances
   the phase by `frequency / sampleRate`.
2. Slides the frequency multiplicatively (equal musical steps, not equal Hz).
3. Multiplies by an **envelope**: a 2 ms fade-in, a quick decay
   (`e^(-3 * progress)`, which makes each note a "blip"), and a 6 ms fade-out.
   Jumping straight to or from full volume makes an audible click; the fades
   prevent it, so every sound starts and ends at exactly silence.
4. Afterwards, a gentle **low-pass filter** (5 kHz, one-pole) takes the harsh edge
   off the square waves.

## The player (`audio/player.cpp`)

Wraps **miniaudio** (`deps/miniaudio`), which mixes on its own thread and feeds the
speakers. The player is only called from the main thread; miniaudio handles the
hand-off.

- `add(samples)` keeps a sound ready to play and returns a `SoundId`. Each sound
  gets **4 voices** (a miniaudio `ma_audio_buffer` reading our samples + an
  `ma_sound` playing it), so it can overlap itself up to 4 times. Past that, the
  oldest voice restarts.
- `play(id, volume, pitch)` seeks a voice to the start and starts it. Nothing is
  allocated while playing.
- `setMasterVolume(v)` scales everything (the SOUND setting).
- **No audio device?** `ma_engine_init` fails, the player prints a warning and
  every call becomes a no-op. The game runs silently.

miniaudio's types stay inside player.cpp: the header only has
`struct Impl; std::unique_ptr<Impl>` (the **pimpl** idiom). The `Voice` objects are
heap-allocated because miniaudio keeps pointers to them - they must never move.

miniaudio is built with decoding, encoding, waveform generation and its resource
manager turned off (we only play our own samples) - see `deps/CMakeLists.txt`.

## The sounds (`app/sounds.cpp`)

| Sfx | Notes | When |
|---|---|---|
| `Start` | C5 E5 G5 C6 rising (Pulse25) | NEW GAME |
| `Step` | a 30 ms low triangle "tuk", alternating two pitches | every tick the snake moves |
| `EatSmall` | G5 C6 | ate small food |
| `EatMedium` | G5 C6 E6 | ate medium food |
| `EatLarge` | G5 C6 E6 G6 C7, thinner (Pulse12) | ate large food |
| `Crash` | noise crunch, then G4 E4 C4 sliding down to G3 | crashed: the round restarts |
| `MenuMove` | one short thin blip | selection moved, or a setting changed |
| `MenuSelect` | E5 A5 | into the settings page, or back to the game |
| `MenuBack` | A5 E5 | out of the settings page, or Escape into the menu |

They're mostly notes of the C major chord (C, E, G), so they always sound good
together; rising runs read as good news, falling ones as bad.

**Levels.** Every sound has its own volume in `kDesigns`. At the default MEDIUM
setting they peak around -19 to -27 dBFS: present but quiet. The step is the
quietest, since it plays 8 times a second.

**Volume setting.** OFF / LOW / MEDIUM / HIGH map to 0, 0.25, 0.5 and 1.0. Each
step is half (-6 dB) of the next, because hearing is logarithmic: equal steps in
amplitude would sound uneven.

## From game events to sounds

The game records events (see [game-rules.md](game-rules.md#events)); each frame
`Application` passes them to `Sounds::playGameEvents()`:

- `Moved` -> Step, `Ate(type)` -> the matching Eat, `Crashed` -> Crash.
- Each sound plays **at most once per frame**. If a stall makes 3 ticks run in one
  frame, that's one step sound, not 3 stacked.
- A crash plays **alone** - no step or eat sound on top of it.

Menu sounds are played by `Application::handleMenuKey()`, which compares the menu
before and after the key (page changed? selection moved?) and looks at the command.

## Listening to a sound alone

`renderSfx(Sfx, sampleRate)` returns any sound's samples. Write them to a `.wav`
file to listen to a change without playing the game (a WAV is a 44-byte header plus
16-bit samples).
