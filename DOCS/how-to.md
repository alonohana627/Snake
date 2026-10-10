# How to...

Step-by-step recipes for common changes. Each lists every file involved, so nothing
is forgotten. Build with zero warnings, and run a debug build to check for
validation messages, after each.

## Add a setting

Example: a "Grid lines: on/off" setting.

1. **[settings.hpp](../include/snake/app/settings.hpp)** - add the field with its
   default to `Settings` (and an `enum class` if it has more than two values).
2. **[menu.hpp](../include/snake/app/menu.hpp)** - add a `MenuItem` value.
3. **[menu.cpp](../src/app/menu.cpp)**
   - add the item to the list in `showSettingsPage()`,
   - add it to the cases in `activate()` that call `change()`,
   - in `change()`, update `settings` and return `MenuCommand::SettingsChanged`.
     For an enum, `_next(value, step, count)` cycles through its values.
4. **[menu_view.cpp](../src/app/menu_view.cpp)** - its label in `_labelOf()`, its
   value text in `_valueOf()`.
5. **[application.cpp](../src/app/application.cpp)** - if something must happen
   when it changes, do it in `applySettings()`. (If a view just reads it, pass
   `m_settings` or the value to the view instead.)
6. **[save_data.cpp](../src/app/save_data.cpp)** - a name table for its values, one
   line in `loadSaveData()` (with the default as fallback) and one in
   `saveSaveData()`.
7. Check the settings panel still fits: each item adds about 72 px of height.

## Add a sound

1. **[sounds.hpp](../include/snake/app/sounds.hpp)** - add an `Sfx` value and bump
   `kSfxCount`.
2. **[sounds.cpp](../src/app/sounds.cpp)**
   - write its notes as a `constexpr Note kMySound[] = {...}`,
   - add `{kMySound, volume}` to `kDesigns` **at the same position** as in the
     enum (the `static_assert` catches a wrong count, not a wrong order).
3. Play it: `m_sounds.play(Sfx::MySound)` in `Application`, or map a game event to
   it in `Sounds::playGameEvents()`.
4. Listen to it alone: write `renderSfx(Sfx::MySound, 48000)` to a WAV file.

Tips for 8-bit sounds: keep notes short (30-100 ms) and from one chord; rising =
good news, falling = bad; `Pulse25` is the classic lead, `Triangle` is soft,
`Noise` with a falling frequency is an impact. Keep volumes around 0.1-0.2.

## Add a food type

1. **[game.hpp](../include/snake/game/game.hpp)** - add a `FoodType` value and its
   cases in `growthOf()` and `pointsOf()`.
2. **[game.cpp](../src/game/game.cpp)** - add its weight to `kFoodWeights`
   (in `FoodType` order).
3. **[game_view.cpp](../src/app/game_view.cpp)** - its color and size in
   `_foodLook()`, and add it to the legend loop in `_appendInfoPanel()`.
4. **[sounds.cpp](../src/app/sounds.cpp)** - an eat sound (see above) and its case
   in `_eatSound()`.

## Add a new shape (a shader)

Example: a `Star` shape.

1. **[types.hpp](../include/snake/core/types.hpp)** - add `Star` to `core::Shape`.
2. **`src/gfx/shaders/star.frag`** - copy `rect.frag` as a start. Keep the
   push-constant block identical to the others (see
   [rendering.md](rendering.md#sprites-and-shaders)); `inUv` (0..1) is the pixel's
   position inside the sprite. Signed distance functions plus `fwidth` give smooth
   edges - `food.frag` is a good example.
3. **[src/gfx/CMakeLists.txt](../src/gfx/CMakeLists.txt)** - add the file to
   `SHADERS`.
4. **[sprite_pipeline.cpp](../src/gfx/sprite_pipeline.cpp)** - include its
   `.spv.inc` into a `kStarFragSpv` array and add the case to `_fragmentShader()`.
5. **[renderer.hpp](../include/snake/gfx/renderer.hpp) / [.cpp](../src/gfx/renderer.cpp)**
   - a `m_starPipeline` member, created in the constructor, and its case in
   `pipelineFor()`.
6. Use it from a view: `{.rect = ..., .color = ..., .shape = core::Shape::Star}`.

## Add a key

1. **[window.hpp](../include/snake/platform/window.hpp)** - add a `Key` value.
2. **[window.cpp](../src/platform/window.cpp)** - map it both ways in
   `_toGlfwKey()` and `_fromGlfwKey()` (`GLFW_KEY_...` constants).
3. **[application.cpp](../src/app/application.cpp)** - handle it in
   `handleInput()`, or in `Menu::handle()` for menu keys.

## Add a character to the font

In **[text.cpp](../src/app/text.cpp)**, add a line to `kFont`: the character and
7 rows of exactly 5 characters, `#` for a lit pixel. The compiler checks the row
widths.

## Add a component

1. Create `include/snake/<name>/` and `src/<name>/`, code in `snake::<name>`.
2. **`src/<name>/CMakeLists.txt`**:

   ```cmake
   # One or two lines on what it does and what it may use.
   snake_add_component(<name>
       first.cpp
   )
   target_link_libraries(snake_<name>
       PRIVATE some_dependency   # only if needed
   )
   ```

3. **[src/CMakeLists.txt](../src/CMakeLists.txt)** - `add_subdirectory(<name>)`
   (before the components that use it) and a line in the tree comment.
4. Link it from whoever uses it (usually `app`), and add it to the component table
   in [architecture.md](architecture.md) and [../README.md](../README.md).

## Change the board size or speed

- Board: `kBoardWidth` / `kBoardHeight` in game.hpp. The view fits any size;
  `kStartHead` in game.cpp is the middle.
- Speeds: `ticksPerSecond()` in settings.hpp.
