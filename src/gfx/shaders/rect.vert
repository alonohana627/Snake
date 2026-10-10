#version 450

// Per-draw data sent from the CPU with vkCmdPushConstants.
// Must match PushConstants in rect_pipeline.cpp.
layout(push_constant) uniform PushConstants {
    vec4 color;
    vec2 position;   // top-left corner, in pixels
    vec2 size;       // in pixels
    vec2 screenSize; // in pixels
} pc;

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
}
