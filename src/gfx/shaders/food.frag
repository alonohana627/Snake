#version 450

// A piece of fruit: a round body with a soft highlight, a short stem and a leaf.
// Built from signed distance functions like snake_head.frag: each shape gives the
// distance to its edge (negative inside), and the pixel is painted accordingly.

// Same block as in sprite.vert - both stages see the same push constants.
layout(push_constant) uniform PushConstants {
    vec4 color;
    vec2 position;
    vec2 size;
    vec2 screenSize;
    int quarterTurns;
} pc;

// This pixel's position inside the cell, 0..1 per axis (y points down).
layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

const vec3 kStemColor = vec3(0.25, 0.12, 0.04);
const vec3 kLeafColor = vec3(0.15, 0.60, 0.15);

float sdCircle(vec2 p, float radius)
{
    return length(p) - radius;
}

// Box centered at 0 with half-size `halfSize`.
float sdBox(vec2 p, vec2 halfSize)
{
    const vec2 d = abs(p) - halfSize;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0);
}

// An ellipse with radii `radii`. Not an exact distance, but close enough for
// small shapes: it's 0 on the edge and has the right sign.
float sdEllipse(vec2 p, vec2 radii)
{
    return (length(p / radii) - 1.0) * min(radii.x, radii.y);
}

// 1 inside the shape, 0 outside, with a soft edge about one pixel wide.
float fill(float distance)
{
    const float pixel = fwidth(distance);
    return 1.0 - smoothstep(-pixel, pixel, distance);
}

void main()
{
    const vec2 p = inUv - 0.5; // -0.5..0.5, the cell's center at 0

    // The fruit sits a little low, leaving room for the stem and leaf on top.
    const float fruit = sdCircle(p - vec2(0.0, 0.06), 0.36);
    const float stem = sdBox(p - vec2(0.0, -0.32), vec2(0.03, 0.08));

    // The leaf: an ellipse next to the stem, tilted by rotating the point the other way.
    const float angle = -0.6;
    const vec2 q = p - vec2(0.13, -0.33);
    const vec2 tilted = vec2(cos(angle) * q.x - sin(angle) * q.y, sin(angle) * q.x + cos(angle) * q.y);
    const float leaf = sdEllipse(tilted, vec2(0.13, 0.06));

    // The fruit: darker toward its outline so it looks round, plus a soft shine top-left.
    const float rim = 1.0 - smoothstep(-0.2, 0.0, fruit); // 1 deep inside, 0 at the edge
    vec3 color = pc.color.rgb * mix(0.6, 1.0, rim);
    const float shine = 1.0 - smoothstep(0.0, 0.08, length(p - vec2(-0.12, -0.06)));
    color = mix(color, vec3(1.0), 0.45 * shine);

    // Stem and leaf on top.
    color = mix(color, kStemColor, fill(stem));
    color = mix(color, kLeafColor, fill(leaf));

    // Transparent wherever none of the shapes are, so the board shows through.
    const float alpha = max(fill(fruit), max(fill(stem), fill(leaf)));
    outColor = vec4(color, pc.color.a * alpha);
}
