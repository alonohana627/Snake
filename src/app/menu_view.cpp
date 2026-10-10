#include "snake/app/menu_view.hpp"

#include "snake/app/text.hpp"

#include <cstddef>
#include <format>
#include <optional>
#include <string>
#include <string_view>

namespace snake::app {

namespace {

// Over the game: a dark veil, so the menu stands out but the board shows through.
constexpr core::Color kVeilColor{.r = 0.0f, .g = 0.0f, .b = 0.0f, .a = 0.65f};

// The panel: a mossy frame (like the board's) around a near-black inside.
constexpr core::Color kPanelFrameColor{.r = 0.06f, .g = 0.11f, .b = 0.07f, .a = 1.0f};
constexpr core::Color kPanelColor{.r = 0.015f, .g = 0.03f, .b = 0.02f, .a = 0.97f};

constexpr core::Color kTitleColor{.r = 0.25f, .g = 0.9f, .b = 0.35f, .a = 1.0f}; // the snake's head
constexpr core::Color kItemColor{.r = 0.55f, .g = 0.65f, .b = 0.55f, .a = 1.0f};
constexpr core::Color kSelectedColor{.r = 1.0f, .g = 0.8f, .b = 0.15f, .a = 1.0f}; // the golden fruit
constexpr core::Color kHintColor{.r = 0.25f, .g = 0.38f, .b = 0.27f, .a = 1.0f};

// Sizes in pixels. Font sizes are font pixels (a character is 5x7 of them).
constexpr float kPanelWidth = 860.0f;
constexpr float kFrameWidth = 6.0f;
constexpr float kPadding = 56.0f;
constexpr float kTitleBoxHeight = 98.0f; // the height of "SNAKE"; shorter titles are centered in it
constexpr float kItemPixels = 6.0f;
constexpr float kItemSpacing = 30.0f; // between two items
constexpr float kHintPixels = 3.0f;
constexpr float kSectionGap = 56.0f;  // title -> items -> hint

} // namespace

namespace { // helpers

std::string_view _labelOf(MenuItem item)
{
    switch (item) {
    case MenuItem::Continue:   return "CONTINUE";
    case MenuItem::NewGame:    return "NEW GAME";
    case MenuItem::Settings:   return "SETTINGS";
    case MenuItem::Quit:       return "QUIT";
    case MenuItem::Fullscreen: return "FULLSCREEN";
    case MenuItem::Speed:      return "SPEED";
    case MenuItem::Back:       return "BACK";
    }
    return "";
}

std::string_view _nameOf(Speed speed)
{
    switch (speed) {
    case Speed::Slow:   return "SLOW";
    case Speed::Normal: return "NORMAL";
    case Speed::Fast:   return "FAST";
    }
    return "";
}

// The setting's current value, for items that are settings.
std::optional<std::string_view> _valueOf(MenuItem item, const Settings& settings)
{
    switch (item) {
    case MenuItem::Fullscreen: return settings.fullscreen ? "ON" : "OFF";
    case MenuItem::Speed:      return _nameOf(settings.speed);
    default:                   return std::nullopt;
    }
}

} // namespace

void appendMenu(std::vector<core::Sprite>& sprites, const Menu& menu, const Settings& settings, core::Size canvas)
{
    const auto canvasWidth = static_cast<float>(canvas.width);
    const auto canvasHeight = static_cast<float>(canvas.height);
    sprites.push_back({.rect = {.x = 0.0f, .y = 0.0f, .width = canvasWidth, .height = canvasHeight}, .color = kVeilColor});

    // The panel's height follows the number of items.
    const float itemHeight = textHeight(kItemPixels);
    const auto itemCount = static_cast<float>(menu.items().size());
    const float itemsHeight = itemCount * itemHeight + (itemCount - 1.0f) * kItemSpacing;
    const float panelHeight = kPadding + kTitleBoxHeight + kSectionGap + itemsHeight + kSectionGap
                            + textHeight(kHintPixels) + kPadding;
    const core::Rect panel{
        .x = (canvasWidth - kPanelWidth) / 2.0f,
        .y = (canvasHeight - panelHeight) / 2.0f,
        .width = kPanelWidth,
        .height = panelHeight,
    };
    const float centerX = panel.x + panel.width / 2.0f;

    sprites.push_back({
        .rect = {.x = panel.x - kFrameWidth, .y = panel.y - kFrameWidth,
                 .width = panel.width + 2.0f * kFrameWidth, .height = panel.height + 2.0f * kFrameWidth},
        .color = kPanelFrameColor,
    });
    sprites.push_back({.rect = panel, .color = kPanelColor});

    // Title: big "SNAKE" on the main page, a smaller "SETTINGS" on the settings page.
    float y = panel.y + kPadding;
    const std::string_view title = menu.onSettingsPage() ? "SETTINGS" : "SNAKE";
    const float titlePixels = menu.onSettingsPage() ? 10.0f : 14.0f;
    appendText(sprites, title, centerX, y + (kTitleBoxHeight - textHeight(titlePixels)) / 2.0f,
               {.pixelSize = titlePixels, .color = kTitleColor, .align = Align::Center, .shadow = true});
    y += kTitleBoxHeight + kSectionGap;

    // Items: the label on the left, a setting's value on the right. The selected
    // one is highlighted, with a ">" in front and "< >" around its value.
    const float labelX = panel.x + 100.0f;
    const float markerX = panel.x + 56.0f;
    const float valueRightX = panel.x + panel.width - 64.0f;
    for (std::size_t index = 0; index < menu.items().size(); ++index) {
        const MenuItem item = menu.items()[index];
        const bool isSelected = static_cast<int>(index) == menu.selected();
        const TextStyle style{.pixelSize = kItemPixels, .color = isSelected ? kSelectedColor : kItemColor, .shadow = true};

        if (isSelected) {
            appendText(sprites, ">", markerX, y, style);
        }
        appendText(sprites, _labelOf(item), labelX, y, style);
        if (const std::optional<std::string_view> value = _valueOf(item, settings)) {
            TextStyle valueStyle = style;
            valueStyle.align = Align::Right;
            const std::string text = isSelected ? std::format("< {} >", *value) : std::string(*value);
            appendText(sprites, text, valueRightX, y, valueStyle);
        }
        y += itemHeight + kItemSpacing;
    }
    y += kSectionGap - kItemSpacing;

    const std::string_view hint = menu.onSettingsPage() ? "LEFT/RIGHT: CHANGE    ESC: BACK"
                                                        : "UP/DOWN: CHOOSE    ENTER: SELECT";
    appendText(sprites, hint, centerX, y, {.pixelSize = kHintPixels, .color = kHintColor, .align = Align::Center});
}

} // namespace snake::app
