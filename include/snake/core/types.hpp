#pragma once

namespace snake::core {

// A width and height in pixels.
struct Size {
    int width = 0;
    int height = 0;
};

// A rectangle in pixels: (0, 0) is the top-left corner, y points down.
struct Rect {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
};

// A color in linear space, 0..1 per channel ("how much light").
// The GPU converts it to sRGB when writing to the screen.
struct Color {
    float r = 0.0f;
    float g = 0.0f;
    float b = 0.0f;
    float a = 1.0f;
};

// What a sprite looks like inside its rectangle. Each shape is its own fragment shader.
enum class Shape {
    Rect,      // solid rectangle (shaders/rect.frag)
    SnakeHead, // snake's head facing +x before turning (shaders/snake_head.frag)
};

// One thing to draw: a shape filling a rectangle on screen.
struct Sprite {
    Rect rect;
    Color color;
    Shape shape = Shape::Rect;
    int quarterTurns = 0; // turns the shape inside the rectangle: clockwise, in 90° steps
};

} // namespace snake::core
