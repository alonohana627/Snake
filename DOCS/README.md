# Snake - developer docs

How the game is put together and why. Start with the architecture, then read
whichever part you're about to change.

| Doc | What's in it |
|---|---|
| [architecture.md](architecture.md) | The components, what each may use, and how a frame flows through them |
| [main-loop.md](main-loop.md) | The main loop: input, fixed-rate ticks, screens (menu / playing), when things are saved |
| [game-rules.md](game-rules.md) | The `game` component: board, snake, turn queue, food, score, walls, events |
| [rendering.md](rendering.md) | The Vulkan renderer from start to finish: setup, one frame, sprites, shaders, the 16:9 canvas |
| [ui.md](ui.md) | Text (the pixel font), the side panels, the menu and how it's drawn |
| [audio.md](audio.md) | The 8-bit synthesizer, playing sounds with miniaudio, which event plays what |
| [persistence.md](persistence.md) | `snake.ini`: where it lives, its format, when it's written, how it survives bad edits |
| [build.md](build.md) | CMake layout, presets, components, shader compilation, dependencies, tests |
| [conventions.md](conventions.md) | Naming, file layout, comments - how code here is written |
| [how-to.md](how-to.md) | Recipes: add a setting, a sound, a food type, a shape/shader, a key, a component |

Installing the tools is in [../INSTALL.md](../INSTALL.md); building and controls in
[../README.md](../README.md).

## Words used throughout

| Word | Meaning |
|---|---|
| **Tick** | One step of the game: the snake moves one cell. Happens at a fixed rate (5, 8 or 12 per second), independent of the frame rate |
| **Frame** | One picture drawn to the screen. Happens at the monitor's refresh rate (vsync) |
| **Cell** | One square of the 20x20 board, `game::Cell{x, y}`, (0, 0) top-left |
| **Canvas** | The fixed 1920x1080 space everything is laid out in, in pixels. Scaled to the window by the renderer |
| **Sprite** | One thing to draw: a rectangle on the canvas, a color and a `Shape` (which fragment shader paints it) |
| **Component** | One folder under `src/` built as a static library (`snake::game`, `snake::gfx`, ...) |
| **View** | Code that turns state into sprites (`game_view`, `menu_view`) or sounds (`sounds`) |
