# Code conventions

How code in this repo is written. New code should look like the code around it.

## Files and namespaces

- One folder per component: headers in `include/snake/<component>/`, sources in
  `src/<component>/`, code in `namespace snake::<component>`. The namespace always
  matches the folder.
- File names are `snake_case` (`fixed_timestep.cpp`, `menu_view.hpp`); a file is
  named after its main class or job.
- Headers start with `#pragma once`.
- Includes, in groups separated by a blank line: the file's own header, then other
  project headers, then third-party, then the standard library - each group
  alphabetical:

  ```cpp
  #include "snake/app/menu_view.hpp"

  #include "snake/app/text.hpp"

  #include <cstddef>
  #include <format>
  ```

## Names

| Kind | Style | Example |
|---|---|---|
| Types, enums, enum values | `PascalCase` | `FixedTimestep`, `Walls::Solid` |
| Functions, variables | `camelCase` | `takePressedKeys()`, `cellSize` |
| Constants | `k` + `PascalCase` | `kBoardWidth`, `kTickSeconds` |
| Member variables | `m_` + `camelCase` | `m_snake`, `m_pendingGrowth` |
| File-private helper functions | `_` + `camelCase` | `_layout()`, `_toGlfwKey()` |

Names say what something *is* in the game's terms (`bestScore`, `nextHead`), not
how it's stored. No abbreviations beyond the obvious (`x`, `y`, `i`).

## File-private code

Things used only inside one `.cpp` go in unnamed namespaces at the top - constants
and types first, then the helpers in their own block marked `// helpers`:

```cpp
namespace snake::app {

namespace {

constexpr float kBoardMargin = 0.75f;

} // namespace

namespace { // helpers

BoardLayout _layout(core::Size area) { ... }

} // namespace

// ... the public functions ...
```

## C++ style

- **C++26**, standard library first. Prefer `std::span`, `std::optional`, ranges,
  `std::println` over hand-rolled versions.
- **`const` by default** for locals that don't change, `constexpr` for values known
  at compile time. Not everything needs to be `constexpr` - use it where it does
  something (tables, constants, compile-time checks like the font).
- **Designated initializers** for structs: `{.x = 1.0f, .y = 2.0f}`. Fields left out
  get their defaults. Vulkan create-infos are written this way too.
- **`enum class`** for every enum. When every value needs its own answer (a name, a
  color, a pipeline), the `switch` lists them all with no `default`, so the compiler
  warns when a value is added; a `return` after the switch satisfies the "no return"
  warning. A `default` is fine when most values mean "nothing to do" (e.g. keys).
- **Ownership is RAII.** A class that owns a resource (a Vulkan handle, a GLFW
  window, the audio engine) frees it in its destructor and deletes copying. Classes
  that *contain* such members are ordered so that construction order = what needs
  what (members are destroyed in reverse).
- **Errors:** setup failures that make the game impossible (no GPU, no window)
  throw `std::runtime_error`, caught in `main`. Things the game can live without (no
  audio device, a save file that can't be written) print a warning and carry on.
  Vulkan results go through `gfx::check()`.
- **Conversions are explicit:** `-Wconversion` is on, so every int/float/size_t
  conversion is a `static_cast`. Builds have zero warnings.
- **Third-party code stays at the edge:** GLFW only in `platform/window.cpp`,
  miniaudio only in `audio/player.cpp`.

## Comments

- Comments explain **why** and **what it means**, especially for anything Vulkan,
  audio, or math - this is a learning project, so err on the side of explaining.
- A comment on a declaration says what it does for the caller; a comment inside a
  function explains a non-obvious step.
- Units go in names or comments: "in pixels", "in cells", "seconds", "Hz".
- No commented-out code, no TODO comments for things that won't be done soon.

## Colors

All colors in code are **linear** RGB (0..1 = amount of light), because the
swapchain is sRGB and converts on write. A color picked in a design tool (sRGB)
must be converted first: linear = ((sRGB + 0.055) / 1.055)^2.4 (or sRGB / 12.92
when sRGB is at most 0.04045).

## Commits

Small, focused commits with a summary line in the imperative ("Add solid walls
option") and a body listing the main changes when there's more than one.
