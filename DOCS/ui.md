# UI: text, side panels and the menu (`app`)

## The views

A **view** turns state into sprites. There are two, both plain functions:

| Function | File | Draws |
|---|---|---|
| `gameDrawList(game, canvas)` | [game_view.cpp](../src/app/game_view.cpp) | Background, board frame, checkerboard, food, snake, both side panels |
| `appendMenu(sprites, menu, settings, canvas)` | [menu_view.cpp](../src/app/menu_view.cpp) | A dark veil over the game, then the menu panel |

They build a new list every frame. A few hundred sprites cost nothing worth
caching, and rebuilding means the picture can never get out of sync with the
state.

All positions are in **canvas pixels** (1920x1080), whatever the window size - the
renderer scales the canvas (see [rendering.md](rendering.md#the-canvas-169-everywhere)).

## Layout of the game screen

```
┌───────────────────────────────── 1920 ─────────────────────────────────┐
│                ┌──────────────────────────────────┐                    │
│    SCORE       │                                  │      FOOD          │
│      12        │                                  │   (red)    +1      │
│                │          20 x 20 board           │   (orange) +2      │
│    BEST        │     (square cells, centered)     │   (golden) +3      │
│     31         │                                  │                    │
│                │                                  │  ARROWS: MOVE      │
│                │                                  │  ESC: MENU         │
│                └──────────────────────────────────┘  F11: FULLSCREEN   │
└────────────────────────────────────────────────────────────────────────┘
```

- **The board** gets the largest square cells that fit the canvas height with a
  0.75-cell margin on each side (`kBoardMargin`): about 50 px per cell at 1080p.
  The frame covers part of that margin: mossy green and 0.3 cells wide, or - with
  solid walls - grey stone and 0.45 cells wide, so the mode reads at a glance.
- **Left panel** (`_appendScorePanel`): SCORE in big digits, BEST below it,
  centered in the space left of the board.
- **Right panel** (`_appendInfoPanel`): the three fruits as drawn on the board with
  their points, and the controls along the bottom.

Colors and sizes are named constants at the top of each view file.

## Text

There's no font file. Characters are a **5x7 pixel font** written as ASCII art in
[text.cpp](../src/app/text.cpp):

```cpp
{'S', {" ####", "#    ", "#    ", " ### ", "    #", "    #", "#### "}},
```

At compile time, `_toBits` turns each character's 35 pixels into the bits of a
64-bit number (bit `row * 5 + column` = pixel on). A row of the wrong width is a
compile error. A lookup table by character code (`kGlyphs`) is built at compile
time too.

**Drawing a character** is one sprite with `Shape::Glyph`, its bits in
`Sprite::glyph`. The bits travel to the GPU in the push constants, and
[glyph.frag](../src/gfx/shaders/glyph.frag) paints a pixel if its bit is on. To
anti-alias when the canvas is scaled, it samples the bit at 4 points spread over
each screen pixel and averages them; at 1:1 scale all 4 agree, so edges stay sharp.

The API ([text.hpp](../include/snake/app/text.hpp)):

```cpp
appendText(sprites, "SCORE", x, y, {
    .pixelSize = 5.0f,         // one font pixel = 5x5 canvas pixels
    .color = kLabelColor,
    .align = Align::Center,    // x is the middle (Left / Center / Right)
    .shadow = true,            // a dark copy behind, offset down-right
});
float textWidth(text, pixelSize);   // to place things next to text
float textHeight(pixelSize);        // 7 * pixelSize
```

Known characters: A-Z (lowercase is drawn as uppercase), 0-9, space and
`: - + . ! / < >`. Unknown characters are blank. Whole-number pixel sizes keep text
sharp, and `appendText` rounds positions to whole pixels for the same reason.

This approach skips everything a "real" font needs in Vulkan (a texture atlas,
image upload, descriptor sets, samplers). The trade-off: one fixed blocky font, and
one draw call per character. Fine for a HUD and a menu.

## The menu

### Logic: `app::Menu`

[menu.hpp](../include/snake/app/menu.hpp) is a small state machine with no drawing:
which **page** is showing, its **items**, and which one is **selected**.

| Page | Items |
|---|---|
| Main | CONTINUE (only when a game is paused), NEW GAME, SETTINGS, QUIT |
| Settings | FULLSCREEN (on/off), SPEED (slow/normal/fast), WALLS (wrap/solid), SOUND (off/low/medium/high), BACK |

`handle(key, settings)`:

| Key | Does |
|---|---|
| Up / Down | Move the selection (wraps around) |
| Enter | Activate: returns a command, opens a page, or steps a setting forward |
| Left / Right | Change the selected setting (cycles through its values) |
| Escape | Settings page: back to main. Main page: continue, if there's a game |

It changes `Settings` directly and returns a `MenuCommand` saying what the
application should do (see [main-loop.md](main-loop.md#screens)).

### Look: `appendMenu`

1. A full-canvas black rectangle at 65% opacity dims the game behind.
2. A panel (mossy frame, near-black inside), centered; its height follows the item
   count.
3. The title: big "SNAKE" or smaller "SETTINGS".
4. Items: label on the left, the setting's value on the right. The selected one is
   golden, with a `>` in front and `< >` around its value - marked by shape as
   well as color.
5. A hint line at the bottom ("UP/DOWN: CHOOSE  ENTER: SELECT").

Labels and value names are mapped in `_labelOf` / `_valueOf` in menu_view.cpp.
