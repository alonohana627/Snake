# Architecture

## The idea

The game's **rules** are kept apart from everything that **shows** them or **talks
to the computer** (window, GPU, speakers, disk). The rules live in `game`, which
depends on nothing. Everything else is built around it and only reads its state or
tells it what to do.

That gives a few things:

- The rules can be tested with plain C++: no window, no GPU, no sound card.
- Changing how the game looks or sounds can't break how it plays.
- Each piece is small enough to read on its own.

## Components

Each folder under `src/` is a **component**: a static library with its headers in
`include/snake/<name>/` and its code in namespace `snake::<name>`.

| Component | Job | Uses | Third-party (private) |
|---|---|---|---|
| `game` | Rules and state: board, snake, food, score, walls, events | nothing | - |
| `core` | Small shared types (`Size`, `Rect`, `Color`, `Sprite`) and `FixedTimestep` | nothing | - |
| `platform` | The window and keyboard | `core` (+ Vulkan's headers, for the surface types) | GLFW |
| `gfx` | The Vulkan renderer: draws a list of sprites | `core`, `platform` | Vulkan |
| `audio` | An 8-bit synthesizer and a sound player | nothing | miniaudio |
| `storage` | The user's data folder, safe file writes, the INI format | nothing | - |
| `app` | Ties it all together: main loop, menu, settings, views, sounds, saving | all of the above | - |

The executable (`src/main.cpp`) only creates `app::Application` and calls `run()`.

```
snake (main.cpp)
└── app                 uses everything below
    ├── game            uses nothing
    ├── audio           uses nothing            (miniaudio inside)
    ├── storage         uses nothing
    ├── gfx             uses platform, core     (Vulkan)
    ├── platform        uses core               (GLFW inside)
    └── core            uses nothing
```

**Rules for dependencies:**

- Arrows only point down. `game`, `core`, `audio` and `storage` depend on no other
  component, so they can be reused or tested alone.
- A third-party library is linked `PRIVATE` by the one component that wraps it,
  and its headers stay out of that component's public headers. GLFW is only
  included in `platform/window.cpp`; miniaudio only in `audio/player.cpp` (through
  a pimpl). Vulkan is the exception: `gfx` exposes Vulkan types in its headers,
  because `gfx` *is* the Vulkan layer.
- `app` is the only component that knows about more than one other component.

> Note: on MSYS2, GLFW and the Vulkan headers are also installed system-wide, so
> `#include <GLFW/glfw3.h>` would compile in any component on that machine. The
> rule is kept by convention there; CMake enforces it on machines without the
> system-wide copies.

## Who owns what

`app::Application` owns everything, as members. C++ creates members top to bottom
and destroys them bottom to top, so the order in `application.hpp` *is* the
lifetime order:

```
platform::Window      created first, destroyed last  (the renderer draws into it)
gfx::Renderer         needs the window
audio::Player         the audio device
app::Sounds           holds a reference to the player
game::Game            the rules and state
app::Settings         what the player chose
core::FixedTimestep   turns real time into ticks
app::Menu             the menu's state
...                   screen, save-file path, last saved data
```

Inside `gfx::Renderer` the same trick orders the Vulkan objects (instance, surface,
device, swapchain, pipelines, frames) - see [rendering.md](rendering.md).

## One frame, end to end

```
Window::pollEvents()              GLFW callbacks queue key presses
      │
Application::handleInput()        keys -> menu (Menu::handle) or game (Game::turn)
      │                           F11 / setting changes -> applySettings() -> save()
      │
FixedTimestep::advance()          how many ticks are due (0, 1, 2...)
Game::tick() x N                  only while Playing
      │
Game::takeEvents()                Moved / Ate / Crashed since last frame
Sounds::playGameEvents()          -> audio::Player::play()
save() on Crashed                 a round ended: maybe a new best score
      │
gameDrawList(game, canvas)        state -> sprites (board, food, snake, side panels)
appendMenu(...)                   + menu sprites, when the menu is open
Renderer::drawFrame(sprites)      Vulkan: record, submit, present (waits for vsync)
```

State only flows one way: `app` reads the game's state and turns it into pictures
and sounds; the game never calls out. When the game needs to say "something
happened", it records an **event** (`game::Event`) that `app` collects - the same
way the window records key presses that `app` collects.

## Where to put new code

| You want to... | Put it in |
|---|---|
| Change a rule (speed, collisions, scoring, food) | `game` |
| Change how something looks | `app/game_view.cpp`, `app/menu_view.cpp`, or a shader in `gfx/shaders/` |
| Change how something sounds | `app/sounds.cpp` |
| Add a setting | `app/settings.hpp`, `app/menu*.cpp`, `app/save_data.cpp` (see [how-to.md](how-to.md)) |
| Talk to the OS (window, input, monitors) | `platform` |
| Draw a new kind of shape | `gfx` (a fragment shader + a `core::Shape` value) |
| Anything about files | `storage` (generic) or `app/save_data.cpp` (what Snake stores) |
