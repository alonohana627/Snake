# The main loop

`app::Application` ([application.cpp](../src/app/application.cpp)) runs the game.

## Starting up

The constructor, in order:

1. Opens the window - **hidden**, 1920x1080, centered on the main monitor.
2. Creates the renderer, the audio player, and renders all sounds.
3. Creates the game with a random seed (`std::random_device`), so food spots differ
   every run.
4. Loads `snake.ini` (settings + best score) and gives the best score to the game.
   On the very first run, writes the file with defaults.
5. `applySettings()`: fullscreen, tick rate, volume, walls.
6. Opens the menu, then **shows** the window - already centered and, if saved,
   fullscreen, so it never visibly jumps.

## One iteration

```cpp
while (!window.shouldClose()) {
    pollEvents();          // GLFW runs our callbacks: key presses are queued
    handleInput();         // each queued key -> menu or game
    if minimized: waitEvents(), reset the clock, continue
    if Playing: run the ticks that are due
    play sounds for the game's events; save if a round ended
    draw the game (and the menu on top, if open)
}
save();                    // the window is closing
```

**Pacing.** Nothing in the loop sleeps. Presenting a frame waits for the monitor's
refresh (vsync, `VK_PRESENT_MODE_FIFO_KHR`), so the loop runs once per refresh: 60
times a second on a 60 Hz monitor, 144 on a 144 Hz one.

**Minimized.** A minimized window has a 0x0 drawing area, and Vulkan can't make a
0x0 swapchain. The loop sleeps in `waitEvents()` until something happens, then
resets the clock so the time spent minimized doesn't turn into a burst of ticks.

## Ticks: same game at any frame rate

The game moves in **ticks** (one cell per tick), at a fixed rate set by the Speed
setting: 5, 8 or 12 per second. Frames happen at whatever rate the monitor runs.
`core::FixedTimestep` ([fixed_timestep.hpp](../include/snake/core/fixed_timestep.hpp))
connects the two:

```
every frame:  unsimulated += time since last frame   (capped at 0.25 s)
              ticks due    = how many whole ticks fit in `unsimulated`
              unsimulated -= ticks due * tick length (the rest carries over)
```

- At 144 FPS and 8 ticks/s, most frames run 0 ticks and about every 18th runs 1.
- At 30 FPS, some frames run 0 and some run 1 - the snake still moves 8 cells/s.
- After a stall (window dragged, debugger pause), the 0.25 s cap stops the game
  from racing through seconds of catch-up ticks at once.
- When the menu opens or closes, `m_ticks.reset(now)` makes sure menu time isn't
  game time.

## Input

`platform::Window` registers a GLFW key callback. It turns GLFW key codes into
`platform::Key` values and appends **presses** (not releases, not auto-repeat) to a
list. `takePressedKeys()` hands the list over and clears it.

Every key is used, in order, even several per frame. That's what makes quick
combos work: Up then Left within one tick queues both turns in the snake (see
[game-rules.md](game-rules.md#turning)).

Routing, in `Application::handleInput()`:

| Key | On any screen | Playing | Menu |
|---|---|---|---|
| F11 | Toggle fullscreen (and save) | | |
| Arrows | | `Game::turn` | `Menu::handle` (move / change setting) |
| Escape | | Pause: open the menu | `Menu::handle` (back / continue) |
| Enter | | - | `Menu::handle` (select) |

## Screens

```
          start
            │
            ▼
   ┌──── Menu ◄────────── Escape ──────┐
   │        │                           │
   │        │ NEW GAME / CONTINUE       │
   │        ▼                           │
   │     Playing ───────────────────────┘
   │
   └─ QUIT -> window closes
```

`Application::m_screen` is `Menu` or `Playing`. The game only ticks while
`Playing`; in the menu it stays frozen and is drawn dimmed behind the panel.
The menu's own pages (main / settings) are inside `app::Menu` - see [ui.md](ui.md).

`Menu::handle()` returns a `MenuCommand` and `handleMenuCommand()` acts on it:

| Command | What the application does |
|---|---|
| `Continue` | Back to `Playing` (clock reset) |
| `NewGame` | `Game::startOver()`, then `Playing` |
| `Quit` | Closes the window |
| `SettingsChanged` | `applySettings()` (the menu already changed `m_settings`) |
| `None` | Nothing (the selection moved or the page changed) |

`handleMenuKey()` also plays the matching menu sound - after acting, so a new
volume is heard right away.

## Settings take effect in one place

`applySettings()` pushes `m_settings` everywhere it matters:

| Setting | Goes to |
|---|---|
| Fullscreen | `Window::setFullscreen` |
| Speed | `FixedTimestep::setStepSeconds` (1 / ticks per second) |
| Walls | `Game::setWalls` |
| Sound | `audio::Player::setMasterVolume` |

Then it calls `save()`.

## When the file is saved

`save()` writes `snake.ini` only if the settings or best score differ from what was
last written. It's called:

- after any settings change (including F11),
- when a round ends (a `Crashed` event) - the best score may have changed,
- when the menu opens (pausing),
- after the loop ends (the window is closing).

Not after every food eaten: the best score changes often during a good round, and
the moments above already cover it. Details in [persistence.md](persistence.md).
