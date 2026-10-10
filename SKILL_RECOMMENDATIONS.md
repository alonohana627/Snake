# Skill recommendations

Skills worth adding to the skills folder (`~/.claude/skills/`) for this project.
Each entry describes the skill well enough to hand to a prompt that writes it.
Kept up to date while developing - see `AGENTS.md`.

## Already installed

Don't recommend these again (or anything they already cover):

- `cpp26-developer`
- `computer-graphics-developer`
- `game-engine-developer`
- `shader-developer`
- `software-architecture-developer`
- `vulkan-developer`

## Recommended

Highest value for the next steps first.

1. **cpp-testing** - Unit tests with Catch2 or doctest plus CTest, testing game logic
   without a window or GPU, testing `constexpr` code with `static_assert`, and
   deterministic tests for tick-based simulations.
   *Why: `tests/` is set up but empty; the game logic is ready to be tested.*

2. **vulkan-text-and-ui** - Drawing text in Vulkan (bitmap or SDF font atlases,
   FreeType/stb_truetype), simple on-screen UI (score, menus, game-over screen),
   and adding Dear ImGui to a Vulkan + GLFW app.
   *Why: the score is shown in the window title because there's no text rendering yet.*

3. **learning-mentor** - A teaching style for someone new to a topic: small steps,
   explain each new concept before using it, short glossary-style definitions,
   "try this next" exercises, and checking understanding before moving on.
   *Why: this is a learning project, and Vulkan and shaders are new to the developer.*

4. **cmake-build-developer** - Modern target-based CMake, presets, git submodules vs
   FetchContent vs vcpkg, toolchains (MSYS2/MSVC/Linux), shader-compile steps, and
   per-component libraries with enforced dependencies.

5. **glfw-platform-developer** - GLFW windows, keyboard/gamepad input, high-DPI and
   framebuffer size, fullscreen/borderless modes, vsync and frame pacing, and the
   cross-platform differences between Windows, Linux and macOS.

6. **game-feel-designer** - Classic arcade game design: Snake/Tetris-style rules,
   difficulty curves, input buffering, "juice" (screen shake, easing, particles,
   feedback on eat/die), and score systems.

7. **cpp-debugging-profiling** - Debugging C++ on Windows with LLDB/GDB and in
   VS Code/Zed, AddressSanitizer/UBSan with clang, RenderDoc for Vulkan frames,
   Tracy profiling, and reading crash dumps.

8. **game-audio-developer** - Sound effects and music with miniaudio or SDL_mixer,
   mixing and volume, and triggering sounds from game events without coupling the
   audio to game logic.

9. **game-persistence** - Saving high scores and settings to disk: file locations
   per OS, simple formats (JSON/TOML/binary), versioning, and safe writes.

10. **ci-cross-platform** - GitHub Actions for C++/CMake/Vulkan on Windows, Linux and
    macOS (MoltenVK), caching, running tests in CI, and packaging a release build.
