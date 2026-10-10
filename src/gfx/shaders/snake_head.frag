#version 450

// A snake's head, drawn inside its rectangle: a snout rounded at the front and
// flat at the back (where the body joins), two eyes looking forward and two
// nostrils. No textures - every pixel works out what it is with a bit of math.
//
// The trick is "signed distance functions" (SDFs): for a shape, a function that
// gives the distance from a point to the shape's edge - negative inside,
// positive outside, 0 on the edge. A pixel is inside if its distance is < 0.

// Same block as in sprite.vert - both stages see the same push constants.
layout(push_constant) uniform PushConstants {
    vec4 color;
    vec2 position;
    vec2 size;
    vec2 screenSize;
    int quarterTurns;
} pc;

// This pixel's position inside the head, 0..1 per axis, already turned so the
// head always faces +x here: the snout is at x = 1, the back at x = 0.
layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

// Distance to a box centered at 0 with half-size `halfSize`, corners rounded with
// radius r.x (front-bottom), r.y (front-top), r.z (back-bottom), r.w (back-top).
// (Front = +x. In these coordinates y points down, so "bottom" is +y.)
float sdRoundedBox(vec2 p, vec2 halfSize, vec4 r)
{
    r.xy = (p.x > 0.0) ? r.xy : r.zw;
    r.x = (p.y > 0.0) ? r.x : r.y;
    const vec2 q = abs(p) - halfSize + r.x;
    return min(max(q.x, q.y), 0.0) + length(max(q, 0.0)) - r.x;
}

float sdCircle(vec2 p, float radius)
{
    return length(p) - radius;
}

// 1 inside the shape, 0 outside, with a soft edge about one pixel wide
// (anti-aliasing). fwidth = how much `distance` changes from one pixel to the next.
float fill(float distance)
{
    const float pixel = fwidth(distance);
    return 1.0 - smoothstep(-pixel, pixel, distance);
}

void main()
{
    const vec2 p = inUv - 0.5; // -0.5..0.5, the head's center at 0

    // The head: round at the snout, nearly square at the back.
    const float head = sdRoundedBox(p, vec2(0.5), vec4(0.45, 0.45, 0.08, 0.08));

    // The head is symmetric, so mirror y: one formula draws both eyes (and both nostrils).
    const vec2 m = vec2(p.x, abs(p.y));
    const float eye = sdCircle(m - vec2(0.08, 0.22), 0.13);
    const float pupil = sdCircle(m - vec2(0.12, 0.22), 0.06); // a bit forward: looking ahead
    const float nostril = sdCircle(m - vec2(0.38, 0.09), 0.025);

    // Base color, darker toward the outline so the head looks rounded.
    const float rim = 1.0 - smoothstep(-0.2, 0.0, head); // 1 deep inside, 0 at the edge
    vec3 color = pc.color.rgb * mix(0.65, 1.0, rim);

    // Paint the details on top, one layer at a time.
    color = mix(color, vec3(0.9), fill(eye));
    color = mix(color, vec3(0.02), fill(pupil));
    color = mix(color, pc.color.rgb * 0.3, fill(nostril));

    // Outside the head's outline the pixel is transparent, so the board shows through.
    outColor = vec4(color, pc.color.a * fill(head));
}
