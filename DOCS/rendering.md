# Rendering (`gfx`)

The renderer has one job: **draw a list of sprites** each frame.
`Renderer::drawFrame(sprites, canvas)` is its whole public interface. Everything
the game looks like is decided elsewhere (`app/*_view.cpp`) and arrives as sprites.

It's written in plain Vulkan 1.3 (the C API, no helper libraries), using the
modern, simpler parts: **dynamic rendering** (no render-pass objects) and
**synchronization2** (clearer barriers).

## Vulkan in one paragraph

Vulkan is a way to give the GPU work. You don't call "draw a rectangle"; you
**record** commands ("use this pipeline", "draw 4 vertices") into a **command
buffer**, **submit** it to a **queue**, and the GPU runs it later, on its own
schedule. Because CPU and GPU run at the same time, you must say explicitly what
waits for what (**semaphores** between GPU jobs, **fences** for the CPU to wait on
the GPU, **barriers** inside a command buffer). The finished picture goes to a
**swapchain**: a few images the screen takes turns showing.

## The objects (set up once)

`Renderer` owns these, as members in this order (created top to bottom, destroyed
bottom to top - each one needs the ones above it):

| Object | File | What it is |
|---|---|---|
| `Instance` | instance.cpp | The connection to Vulkan. Debug builds turn on the validation layer (`VK_LAYER_KHRONOS_validation`) and print its messages as `[Vulkan] ...` |
| `Surface` | surface.cpp | Vulkan's handle to our window (made by GLFW, through `platform::Window`) |
| `Device` | device.cpp | Picks the GPU (must have Vulkan 1.3, swapchain support, dynamic rendering, synchronization2; prefers a discrete card) and opens it with one queue that can both draw and present |
| `Swapchain` | swapchain.cpp | The images shown on screen: sRGB format, vsync (`FIFO`), one more image than the minimum. Also one "render finished" semaphore per image |
| `SpritePipeline` x5 | sprite_pipeline.cpp | One per `Shape`: the shaders plus every drawing setting, baked into one object |
| `Frame` x2 | frame.cpp | What one frame in flight needs: a command pool + buffer, an "image acquired" semaphore, an "in flight" fence |

`check(result, "what")` ([check.hpp](../include/snake/gfx/check.hpp)) turns any
failed `VkResult` into an exception with a readable message.

## One frame (`drawFrame`)

```
1. Window resized?            -> recreate the swapchain
2. frame.waitUntilFree()      CPU waits on this frame's fence: the GPU is done
                              with the last use of this command buffer
3. vkAcquireNextImageKHR      which swapchain image to draw into; signals
                              imageAcquired when it's really free
                              (OUT_OF_DATE -> recreate and skip this frame)
4. frame.reset()              fence back to unsignaled, command buffer emptied
5. recordCommands()           see below
6. submit                     waits on imageAcquired, signals renderFinished[image]
                              and the frame's fence
7. vkQueuePresentKHR          shows the image once renderFinished is signaled
                              (OUT_OF_DATE / SUBOPTIMAL -> recreate)
```

**Two frames in flight**: while the GPU draws frame N, the CPU already records
frame N+1 into the other `Frame`. Step 2 makes sure the CPU never reuses a
`Frame` the GPU is still reading.

**Why "render finished" is per swapchain image**, not per frame: presentation may
still be waiting on it when the same frame slot comes around again; one per image
guarantees it's free when reused.

### Recording the commands

```
barrier:  image UNDEFINED -> COLOR_ATTACHMENT_OPTIMAL      (ready to draw into)
vkCmdBeginRendering    clear to black (the letterbox color)
set viewport + scissor to the letterbox rectangle (see "The canvas")
for each sprite:
    bind its shape's pipeline (only if different from the previous sprite)
    push constants (color, rectangle, ...) and draw 4 vertices
vkCmdEndRendering
barrier:  image COLOR_ATTACHMENT_OPTIMAL -> PRESENT_SRC    (ready to show)
```

Sprites are drawn **in list order**, later ones on top - so the views build their
lists back to front. There's no depth buffer; order is all there is.

### Resizing and minimizing

The swapchain's images have a fixed size, so a resized window needs a new
swapchain. `recreateSwapchain()` waits for the GPU to go idle, creates the new
one (passing the old one so the driver can reuse resources), then destroys the
old. A 0x0 window (minimized) can't have a swapchain; the renderer skips it and
the main loop waits.

## Sprites and shaders

A sprite ([types.hpp](../include/snake/core/types.hpp)):

```cpp
struct Sprite {
    Rect rect;              // where, in canvas pixels
    Color color;            // linear RGBA
    Shape shape;            // Rect, SnakeHead, Food, Background, Glyph
    int quarterTurns;       // rotate the shape in 90° steps (the snake's head)
    GlyphBits glyph;        // Glyph only: which pixels of the character are on
};
```

**No vertex buffers.** Every sprite is 4 vertices drawn as a triangle strip; the
vertex shader [sprite.vert](../src/gfx/shaders/sprite.vert) makes the corners from
`gl_VertexIndex` (0..3 -> (0,0) (1,0) (0,1) (1,1)), scales them to the sprite's
rectangle and converts canvas pixels to Vulkan's -1..1 coordinates. It also passes
each pixel's position *inside* the sprite (`uv`, 0..1) to the fragment shader,
rotated by `quarterTurns`.

**Per-sprite data travels as push constants**: a small block (56 bytes) set with
`vkCmdPushConstants` right before each draw. Its layout must match the GLSL block
byte for byte; `static_assert`s in sprite_pipeline.cpp check every offset:

| Bytes | Field | Notes |
|---|---|---|
| 0-15 | `vec4 color` | |
| 16-23 | `vec2 position` | top-left, canvas pixels |
| 24-31 | `vec2 size` | |
| 32-39 | `vec2 screenSize` | the canvas size |
| 40-43 | `int quarterTurns` | |
| 44-47 | (padding) | GLSL aligns the next `uvec2` to 8 bytes |
| 48-55 | `uvec2 glyph` | 64 glyph bits as two 32-bit halves |

**One fragment shader per shape** decides the color of each pixel inside the
rectangle. They're all in [src/gfx/shaders/](../src/gfx/shaders/):

| Shape | Shader | Draws |
|---|---|---|
| `Rect` | rect.frag | A solid color (board cells, snake body, panels, the menu veil) |
| `SnakeHead` | snake_head.frag | A rounded snout with eyes, pupils and nostrils |
| `Food` | food.frag | A round fruit with a stem, a leaf and a shine |
| `Background` | background.frag | The color fading darker toward the edges (a vignette) |
| `Glyph` | glyph.frag | One 5x7 pixel-font character (see [ui.md](ui.md#text)) |

The head and fruit are drawn with **signed distance functions**: for each pixel,
compute the distance to a shape's edge (negative inside), and color by that. The
edge is softened over one screen pixel (`fwidth`), which anti-aliases it at any
size.

**Blending** is on in every pipeline (`src * alpha + dst * (1 - alpha)`), so shapes
can have soft edges and translucent sprites (the menu's dark veil) work.

**Shaders are compiled at build time** by `glslc` into SPIR-V, written as a list
of numbers, and `#include`d into arrays in sprite_pipeline.cpp. They're inside
`snake.exe` - no files to find at runtime. See [build.md](build.md#shaders).

## The canvas (16:9 everywhere)

The views lay everything out on a fixed **1920x1080 canvas**, never on the
window directly. The renderer fits the canvas into whatever the window is:

```
scale    = min(window width / 1920, window height / 1080)
viewport = 1920*scale x 1080*scale, centered, whole pixels
```

The **viewport** is the rectangle of the image that the shaders' -1..1 range maps
to; pointing it at that centered rectangle scales the whole canvas there. The
**scissor** clips to the same rectangle. The rest stays at the clear color: black
bars (letterboxing) when the window isn't 16:9.

The window keeps 16:9 when resized by hand (`glfwSetWindowAspectRatio`), and a
16:9 monitor has no bars in fullscreen - the bars only show on other shapes.

## Color

- The swapchain uses an **sRGB** format (`B8G8R8A8_SRGB` or `R8G8B8A8_SRGB`), so the GPU converts the
  shaders' output from linear light to sRGB when writing. That's why all colors in
  the code are **linear** values: `0.5` is half the light, which looks lighter than
  "middle grey" on screen.
- Blending happens in linear space, which is the physically right place for it.

## Validation

Debug builds enable the Khronos validation layer when it's installed (see
[../INSTALL.md](../INSTALL.md)). It checks every Vulkan call against the spec and
prints problems as `[Vulkan] ...`. The game is expected to run with **zero**
messages; any message is a bug, even if the picture looks right.
