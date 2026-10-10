# Snake

Toy project. C++26, CMake + Ninja, Vulkan 1.3 + GLFW.

## Layout

Each component is a static library (`snake::<name>`), with headers in
`include/snake/<name>/`, sources in `src/<name>/` and namespace `snake::<name>`.

| Component  | What it does                                   | Uses             |
|------------|------------------------------------------------|------------------|
| `core`     | Shared types (`Rect`, `Color`), `FixedTimestep` | -                |
| `platform` | Window + keyboard (GLFW)                       | core             |
| `gfx`      | Vulkan renderer, shaders in `src/gfx/shaders/` | core, platform   |
| `game`     | Game state and rules, fixed tick rate          | core             |

`src/main.cpp` wires them together: read input, run due game ticks, draw.

## Build

GLFW lives in `deps/glfw` as a git submodule. After cloning:

```bash
git submodule update --init
```

```bash
Scripts/build.sh [debug|release] [--tests] [--clean]
```

```powershell
Scripts\build.ps1 [-Config Debug|Release] [-Tests] [-Clean]
```

Or directly with presets: `cmake --preset debug && cmake --build --preset debug`.

Output: `build/<preset>/bin/snake`.

Debug builds turn on the Vulkan validation layer if the loader can find it. With
MSYS2 on Windows, point it there once:

```bash
setx VK_ADD_LAYER_PATH "C:\msys64\clang64\bin"
```

## Tests

Off by default. `--tests` / `-Tests` (or the `*-tests` presets) turns on
`SNAKE_BUILD_TESTS`, which adds `tests/` and runs `ctest`. No tests exist yet.
