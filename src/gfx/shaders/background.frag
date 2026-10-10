#version 450

// The window's background: the given color in the middle, fading darker toward
// the edges and corners (a "vignette"), so the eye goes to the board in the center.

// Same block as in sprite.vert - both stages see the same push constants.
layout(push_constant) uniform PushConstants {
    vec4 color;
    vec2 position;
    vec2 size;
    vec2 screenSize;
    int quarterTurns;
} pc;

// This pixel's position in the background rectangle, 0..1 per axis.
layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

void main()
{
    vec2 p = inUv - 0.5; // -0.5..0.5, the center at 0
    p.x *= pc.size.x / pc.size.y; // measure in the same units across and down, so the fade is round

    const float fromCenter = length(p);
    const float light = 1.0 - smoothstep(0.15, 0.95, fromCenter); // 1 near the center, 0 far out
    outColor = vec4(pc.color.rgb * mix(0.3, 1.0, light), pc.color.a);
}
