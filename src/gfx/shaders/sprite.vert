#version 450

// The vertex shader for every sprite: places a rectangle on screen. What's drawn
// inside it is up to the fragment shader (rect.frag, snake_head.frag, ...).

// Per-draw data sent from the CPU with vkCmdPushConstants.
// Must match PushConstants in sprite_pipeline.cpp.
layout(push_constant) uniform PushConstants {
    vec4 color;
    vec2 position;     // top-left corner, in pixels
    vec2 size;         // in pixels
    vec2 screenSize;   // in pixels
    int quarterTurns;  // how the shape is turned inside the rectangle: clockwise, in 90° steps
} pc;

// Where this vertex is inside the shape, 0..1 per axis, in the shape's own
// coordinates (facing +x). The GPU blends it across the triangle, so every
// pixel in the fragment shader gets its own position inside the shape.
layout(location = 0) out vec2 outUv;

void main()
{
    // No vertex buffer: build the rectangle's corners from the vertex index.
    // Drawn as a triangle strip of 4 vertices, indices 0..3 give the corners
    // (0,0) (1,0) (0,1) (1,1) - two triangles that cover the rectangle.
    const vec2 corner = vec2(gl_VertexIndex & 1, gl_VertexIndex >> 1);

    const vec2 pixel = pc.position + corner * pc.size;

    // Pixels -> Vulkan's normalized device coordinates: x and y go from -1 to 1,
    // (-1, -1) is the top-left corner and y points down - same as pixels.
    const vec2 ndc = pixel / pc.screenSize * 2.0 - 1.0;

    gl_Position = vec4(ndc, 0.0, 1.0);

    // Rotate the corner around the rectangle's center by -quarterTurns * 90°.
    // Fragment shaders can then always draw their shape facing +x (right),
    // and it shows up turned the right way on screen.
    const float angle = float(pc.quarterTurns) * 1.5707963; // 90° in radians
    const vec2 c = corner - 0.5;
    outUv = vec2(cos(angle) * c.x + sin(angle) * c.y,
                 -sin(angle) * c.x + cos(angle) * c.y) + 0.5;
}
