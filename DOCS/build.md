# Build system

CMake (3.28+) generates a Ninja build. What to install is in
[../INSTALL.md](../INSTALL.md); this is how the build itself is organized.

## Files

```
CMakeLists.txt              project, C++26, options; adds deps/, src/, tests/
CMakePresets.json           debug, release, debug-tests, release-tests
cmake/
  CompilerWarnings.cmake    snake_set_warnings(target)
  SnakeComponent.cmake      snake_add_component(name sources...)
deps/CMakeLists.txt         GLFW and miniaudio (git submodules)
src/CMakeLists.txt          finds Vulkan, adds each component, the executable
src/<component>/CMakeLists.txt
tests/CMakeLists.txt        the (empty for now) test suite
Scripts/build.sh, build.ps1 configure + build (+ test) in one go
```

## Presets

| Preset | Build type | Tests |
|---|---|---|
| `debug` | Debug: no optimization, debug info, validation layer on | off |
| `release` | Release: optimized, validation off (`NDEBUG`) | off |
| `debug-tests`, `release-tests` | as above | `SNAKE_BUILD_TESTS=ON` |

Each preset builds into its own folder, `build/<preset>/`, and executables land in
`build/<preset>/bin/`. Folders don't interfere, so debug and release can both stay
built.

```bash
cmake --preset debug            # configure (once, or after CMake files change)
cmake --build --preset debug    # build
ctest --preset debug-tests      # run tests (with a -tests preset)
```

`Scripts/build.sh [debug|release] [--tests] [--clean]` and
`Scripts\build.ps1 [-Config Debug|Release] [-Tests] [-Clean]` do the same.

`CMAKE_EXPORT_COMPILE_COMMANDS` writes `compile_commands.json`, which clangd (and
so editors like Zed and VS Code) reads to understand the code; `.clangd` points it
at `build/debug`.

## Components

Each component is created with one call
([SnakeComponent.cmake](../cmake/SnakeComponent.cmake)):

```cmake
snake_add_component(game
    game.cpp
    snake.cpp
)
```

which makes a static library `snake_game`, an alias `snake::game`, adds
`include/` to its public include path and applies the warnings. What a component
may use is exactly what it links:

```cmake
target_link_libraries(snake_platform
    PUBLIC snake::core Vulkan::Headers   # users of platform get these too
    PRIVATE glfw                         # only platform's own .cpp files see GLFW
)
```

- **PUBLIC**: needed by the component's headers, so anyone including them needs it.
- **PRIVATE**: only the component's `.cpp` files use it - it doesn't leak.

The current links ([architecture.md](architecture.md) has the picture):

| Component | PUBLIC | PRIVATE |
|---|---|---|
| core | - | - |
| game | - | - |
| platform | core, Vulkan headers | glfw |
| gfx | core, Vulkan | platform |
| audio | - | miniaudio |
| storage | - | - |
| app | core, platform, gfx, audio, storage, game | - |

## Compiler settings

- **C++26**, no compiler extensions (`CMAKE_CXX_STANDARD 26`, `..._EXTENSIONS OFF`).
- **Warnings** ([CompilerWarnings.cmake](../cmake/CompilerWarnings.cmake)):
  `-Wall -Wextra -Wpedantic -Wconversion -Wshadow -Wnon-virtual-dtor
  -Wold-style-cast -Woverloaded-virtual` (MSVC: `/W4 /permissive-`). Builds are
  expected to have **zero** warnings. `-Wconversion` is the strict one: every
  int/float/size conversion must be written out with `static_cast`.
- On Clang, `-Wno-missing-designated-field-initializers`: Vulkan structs are filled
  with designated initializers that leave the rest zero on purpose
  (`VkFooCreateInfo{.sType = ..., .size = ...}`).
- The third-party libraries are built without our warnings - their code isn't ours
  to fix.

## Shaders

The GLSL shaders in `src/gfx/shaders/` are compiled during the build
([src/gfx/CMakeLists.txt](../src/gfx/CMakeLists.txt)):

```
glslc --target-env=vulkan1.3 -mfmt=num -o rect.frag.spv.inc rect.frag
```

`-mfmt=num` writes the SPIR-V as comma-separated numbers, which
sprite_pipeline.cpp `#include`s inside an array:

```cpp
constexpr std::uint32_t kRectFragSpv[] = {
#include "rect.frag.spv.inc"
};
```

So the shaders are part of `snake.exe`. Editing a shader rebuilds just that file
and sprite_pipeline.cpp. A GLSL error fails the build with glslc's message.

`find_package(Vulkan REQUIRED COMPONENTS glslc)` in src/CMakeLists.txt finds both
the Vulkan library and the `glslc` program (`Vulkan::glslc`).

## Dependencies (`deps/`)

Both are **git submodules**: their source is part of the checkout, pinned to an
exact version, and built with the game. Nothing to install.

| Library | Version | Built how |
|---|---|---|
| GLFW | 3.5.1 | Its own CMakeLists (docs and install rules off) |
| miniaudio | 0.11.25 | Our own small target from its single `miniaudio.c`, with `MA_NO_DECODING`, `MA_NO_ENCODING`, `MA_NO_GENERATION`, `MA_NO_RESOURCE_MANAGER` |

The miniaudio defines are `PUBLIC`: they change the size of miniaudio's structs, so
our code must see the same ones when it includes `miniaudio.h`.

If a submodule folder is empty, configuring stops with "Run: git submodule update
--init".

Updating one: `git -C deps/glfw fetch --tags && git -C deps/glfw checkout <tag>`,
then commit the new submodule pointer.

## Tests

`SNAKE_BUILD_TESTS=ON` (the `-tests` presets, or `--tests` / `-Tests`) adds
`tests/`. There are no tests yet; [tests/CMakeLists.txt](../tests/CMakeLists.txt)
shows how to add one. Link only the component under test - the game, synth and INI
reader need no window, GPU or sound card:

```cmake
add_executable(snake_tests game_test.cpp)
snake_set_warnings(snake_tests)
target_link_libraries(snake_tests PRIVATE snake::game)
add_test(NAME snake_tests COMMAND snake_tests)
```
