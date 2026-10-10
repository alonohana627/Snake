#version 450

// Same block as in rect.vert - both stages see the same push constants.
layout(push_constant) uniform PushConstants {
    vec4 color;
    vec2 position;
    vec2 size;
    vec2 screenSize;
} pc;

layout(location = 0) out vec4 outColor;

void main()
{
    outColor = pc.color;
}
