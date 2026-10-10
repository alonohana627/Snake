#version 450

// One character of a 5x7 pixel font. The CPU sends which of the 35 pixels are on
// (`glyph`, one bit per pixel); this shader paints the "on" pixels and leaves the
// rest transparent. No font texture needed - the whole character fits in 64 bits.

// Same block as in sprite.vert, plus the glyph bits at the end.
layout(push_constant) uniform PushConstants {
    vec4 color;
    vec2 position;
    vec2 size;
    vec2 screenSize;
    int quarterTurns;
    uvec2 glyph; // bit (row * 5 + column): x holds bits 0-31, y bits 32-63
} pc;

// This pixel's position inside the character, 0..1 per axis (y points down).
layout(location = 0) in vec2 inUv;

layout(location = 0) out vec4 outColor;

const ivec2 kGlyphSize = ivec2(5, 7); // columns, rows

// 1.0 if the font pixel at `p` (in font pixels, 0..5 x 0..7) is on, else 0.0.
float fontPixel(vec2 p)
{
    const ivec2 cell = ivec2(floor(p));
    if (any(lessThan(cell, ivec2(0))) || any(greaterThanEqual(cell, kGlyphSize))) {
        return 0.0;
    }
    const int bit = cell.y * kGlyphSize.x + cell.x;
    const uint word = bit < 32 ? pc.glyph.x : pc.glyph.y;
    return float((word >> uint(bit % 32)) & 1u);
}

void main()
{
    const vec2 p = inUv * vec2(kGlyphSize);

    // Anti-aliasing: look at 4 points spread across this screen pixel and average.
    // When font pixels line up with screen pixels (the usual case), all 4 agree and
    // edges are sharp; when the window is scaled, edge pixels get partial coverage
    // instead of jagged steps. dFdx/dFdy: how much `p` changes per screen pixel.
    const vec2 dx = dFdx(p) * 0.25;
    const vec2 dy = dFdy(p) * 0.25;
    const float coverage = (fontPixel(p - dx - dy) + fontPixel(p + dx - dy)
                          + fontPixel(p - dx + dy) + fontPixel(p + dx + dy)) * 0.25;

    outColor = vec4(pc.color.rgb, pc.color.a * coverage);
}
