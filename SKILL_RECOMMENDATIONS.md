# Skill recommendations

Skills worth adding to the skills folder (`~/.claude/skills/`) for this project.
Each entry describes the skill well enough to hand to a prompt that writes it.
Kept up to date while developing - see `AGENTS.md`.

## Already installed

Don't recommend these again (or anything they already cover):

- `ci-cross-platform`
- `cmake-build-developer`
- `computer-graphics-developer`
- `cpp-debugging-profiling`
- `cpp-testing`
- `cpp26-developer`
- `game-audio-developer`
- `game-engine-developer`
- `game-feel-designer`
- `game-persistence`
- `glfw-platform-developer`
- `learning-mentor`
- `shader-developer`
- `software-architecture-developer`
- `vulkan-developer`
- `vulkan-text-and-ui`

## Recommended

Highest value for the next steps first.

1. **game-visual-design** - How a small 2D game should look, as opposed to how to
   draw it: picking a coherent palette (a few hues, value steps for background /
   board / actors / UI, checked for contrast and color blindness), choosing colors in
   sRGB and converting to linear for an sRGB swapchain, visual hierarchy (the player's
   eye goes to the snake and food first, the HUD second, the background last), HUD and
   menu layout on a fixed 16:9 canvas (safe margins, alignment grid, consistent
   spacing and type scale), pixel-art consistency (one pixel size, no mixed
   resolutions), and how to review a screenshot for these issues.
   *Why: colors, sizes and spacing in game_view.cpp and menu_view.cpp were picked by
   eye and adjusted from screenshots; none of the installed skills covers art
   direction.*
