#include "snake/app/text.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <string_view>

namespace snake::app {

namespace {

using core::GlyphBits;
using core::kGlyphColumns;
using core::kGlyphRows;

// One empty font pixel between characters.
constexpr int kAdvanceColumns = kGlyphColumns + 1;

// How dark a text shadow is (its alpha, for fully opaque text).
constexpr float kShadowAlpha = 0.6f;

// A character drawn as text: 7 rows of 5 columns, '#' = pixel on.
struct GlyphArt {
    char character;
    std::array<std::string_view, kGlyphRows> rows;
};

// clang-format off
constexpr GlyphArt kFont[] = {
    {'A', {" ### ", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"}},
    {'B', {"#### ", "#   #", "#   #", "#### ", "#   #", "#   #", "#### "}},
    {'C', {" ### ", "#   #", "#    ", "#    ", "#    ", "#   #", " ### "}},
    {'D', {"#### ", "#   #", "#   #", "#   #", "#   #", "#   #", "#### "}},
    {'E', {"#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#####"}},
    {'F', {"#####", "#    ", "#    ", "#### ", "#    ", "#    ", "#    "}},
    {'G', {" ### ", "#   #", "#    ", "# ###", "#   #", "#   #", " ####"}},
    {'H', {"#   #", "#   #", "#   #", "#####", "#   #", "#   #", "#   #"}},
    {'I', {" ### ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "}},
    {'J', {"  ###", "   # ", "   # ", "   # ", "   # ", "#  # ", " ##  "}},
    {'K', {"#   #", "#  # ", "# #  ", "##   ", "# #  ", "#  # ", "#   #"}},
    {'L', {"#    ", "#    ", "#    ", "#    ", "#    ", "#    ", "#####"}},
    {'M', {"#   #", "## ##", "# # #", "# # #", "#   #", "#   #", "#   #"}},
    {'N', {"#   #", "#   #", "##  #", "# # #", "#  ##", "#   #", "#   #"}},
    {'O', {" ### ", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "}},
    {'P', {"#### ", "#   #", "#   #", "#### ", "#    ", "#    ", "#    "}},
    {'Q', {" ### ", "#   #", "#   #", "#   #", "# # #", "#  # ", " ## #"}},
    {'R', {"#### ", "#   #", "#   #", "#### ", "# #  ", "#  # ", "#   #"}},
    {'S', {" ####", "#    ", "#    ", " ### ", "    #", "    #", "#### "}},
    {'T', {"#####", "  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "  #  "}},
    {'U', {"#   #", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "}},
    {'V', {"#   #", "#   #", "#   #", "#   #", "#   #", " # # ", "  #  "}},
    {'W', {"#   #", "#   #", "#   #", "# # #", "# # #", "# # #", " # # "}},
    {'X', {"#   #", "#   #", " # # ", "  #  ", " # # ", "#   #", "#   #"}},
    {'Y', {"#   #", "#   #", " # # ", "  #  ", "  #  ", "  #  ", "  #  "}},
    {'Z', {"#####", "    #", "   # ", "  #  ", " #   ", "#    ", "#####"}},
    {'0', {" ### ", "#   #", "#  ##", "# # #", "##  #", "#   #", " ### "}},
    {'1', {"  #  ", " ##  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "}},
    {'2', {" ### ", "#   #", "    #", "   # ", "  #  ", " #   ", "#####"}},
    {'3', {"#####", "   # ", "  #  ", "   # ", "    #", "#   #", " ### "}},
    {'4', {"   # ", "  ## ", " # # ", "#  # ", "#####", "   # ", "   # "}},
    {'5', {"#####", "#    ", "#### ", "    #", "    #", "#   #", " ### "}},
    {'6', {"  ## ", " #   ", "#    ", "#### ", "#   #", "#   #", " ### "}},
    {'7', {"#####", "    #", "   # ", "  #  ", " #   ", " #   ", " #   "}},
    {'8', {" ### ", "#   #", "#   #", " ### ", "#   #", "#   #", " ### "}},
    {'9', {" ### ", "#   #", "#   #", " ####", "    #", "   # ", " ##  "}},
    {':', {"     ", "  #  ", "  #  ", "     ", "  #  ", "  #  ", "     "}},
    {'-', {"     ", "     ", "     ", " ### ", "     ", "     ", "     "}},
    {'+', {"     ", "  #  ", "  #  ", "#####", "  #  ", "  #  ", "     "}},
    {'.', {"     ", "     ", "     ", "     ", "     ", "     ", "  #  "}},
    {'!', {"  #  ", "  #  ", "  #  ", "  #  ", "  #  ", "     ", "  #  "}},
    {'/', {"    #", "    #", "   # ", "  #  ", " #   ", "#    ", "#    "}},
    {'<', {"   # ", "  #  ", " #   ", "#    ", " #   ", "  #  ", "   # "}},
    {'>', {" #   ", "  #  ", "   # ", "    #", "   # ", "  #  ", " #   "}},
};
// clang-format on

} // namespace

namespace { // helpers

// The art as bits (see core::GlyphBits). constexpr: done by the compiler, and a
// row of the wrong width is a compile error instead of a garbled character.
constexpr GlyphBits _toBits(const GlyphArt& art)
{
    GlyphBits bits = 0;
    for (int row = 0; row < kGlyphRows; ++row) {
        const std::string_view line = art.rows[static_cast<std::size_t>(row)];
        if (line.size() != kGlyphColumns) {
            throw "every row of a glyph must be exactly 5 characters wide";
        }
        for (int column = 0; column < kGlyphColumns; ++column) {
            if (line[static_cast<std::size_t>(column)] == '#') {
                bits |= GlyphBits{1} << (row * kGlyphColumns + column);
            }
        }
    }
    return bits;
}

// Every ASCII character's bits, looked up by its code. Characters not in kFont
// (including space) stay 0: nothing drawn.
constexpr std::array<GlyphBits, 128> kGlyphs = [] {
    std::array<GlyphBits, 128> glyphs{};
    for (const GlyphArt& art : kFont) {
        glyphs[static_cast<std::size_t>(art.character)] = _toBits(art);
    }
    return glyphs;
}();

GlyphBits _glyphOf(char character)
{
    if (character >= 'a' && character <= 'z') {
        character = static_cast<char>(character - 'a' + 'A');
    }
    const auto code = static_cast<unsigned char>(character);
    return code < kGlyphs.size() ? kGlyphs[code] : 0;
}

} // namespace

float textWidth(std::string_view text, float pixelSize)
{
    if (text.empty()) {
        return 0.0f;
    }
    // Every character plus its gap, minus the gap after the last one.
    const auto columns = static_cast<float>(text.size() * kAdvanceColumns - 1);
    return columns * pixelSize;
}

float textHeight(float pixelSize)
{
    return kGlyphRows * pixelSize;
}

void appendText(std::vector<core::Sprite>& sprites, std::string_view text, float x, float y, const TextStyle& style)
{
    // The shadow first, so the text is drawn on top of it.
    if (style.shadow) {
        TextStyle shadowStyle = style;
        shadowStyle.shadow = false;
        shadowStyle.color = {.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = kShadowAlpha * style.color.a};
        const float offset = std::max(1.0f, std::round(style.pixelSize * 0.5f));
        appendText(sprites, text, x + offset, y + offset, shadowStyle);
    }

    const float width = textWidth(text, style.pixelSize);
    if (style.align == Align::Center) {
        x -= width / 2.0f;
    } else if (style.align == Align::Right) {
        x -= width;
    }
    // Start on a whole pixel, so font pixels line up with screen pixels (sharp edges).
    x = std::round(x);
    y = std::round(y);

    const float advance = kAdvanceColumns * style.pixelSize;
    for (const char character : text) {
        if (const GlyphBits glyph = _glyphOf(character); glyph != 0) {
            sprites.push_back({
                .rect = {.x = x, .y = y, .width = kGlyphColumns * style.pixelSize, .height = kGlyphRows * style.pixelSize},
                .color = style.color,
                .shape = core::Shape::Glyph,
                .glyph = glyph,
            });
        }
        x += advance;
    }
}

} // namespace snake::app
