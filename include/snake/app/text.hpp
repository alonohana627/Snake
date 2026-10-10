#pragma once

#include "snake/core/types.hpp"

#include <string_view>
#include <vector>

namespace snake::app {

// Text in a 5x7 pixel font: each character is one Shape::Glyph sprite.
// Knows A-Z (lowercase is drawn as uppercase), 0-9, space and : - + . ! / < >.
// Unknown characters are drawn as blanks.

enum class Align {
    Left,   // x is where the text starts
    Center, // x is the text's middle
    Right,  // x is where the text ends
};

struct TextStyle {
    float pixelSize = 4.0f; // size of one font pixel. Whole numbers keep the text sharp.
    core::Color color;
    Align align = Align::Left;
    bool shadow = false; // a soft dark copy behind it, down-right, so it reads on any background
};

// Width and height of `text` in pixels.
float textWidth(std::string_view text, float pixelSize);
float textHeight(float pixelSize);

// Adds the sprites that draw `text` with its top edge at y (and x placed by style.align).
void appendText(std::vector<core::Sprite>& sprites, std::string_view text, float x, float y, const TextStyle& style);

} // namespace snake::app
