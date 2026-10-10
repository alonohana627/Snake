#include "snake/app/menu.hpp"

#include <algorithm>
#include <cstddef>
#include <utility>

namespace snake::app {

namespace {

using platform::Key;

constexpr int kSpeedCount = 3;  // Slow, Normal, Fast
constexpr int kVolumeCount = 4; // Off, Low, Medium, High

} // namespace

namespace { // helpers

// An enum value moved `step` places along its `count` values, wrapping around at both
// ends (e.g. Speed: Fast + 1 = Slow).
template <typename Enum>
Enum _next(Enum value, int step, int count)
{
    const int index = (std::to_underlying(value) + step + count) % count;
    return static_cast<Enum>(index);
}

} // namespace

void Menu::open(bool canContinue)
{
    m_canContinue = canContinue;
    showMainPage(canContinue ? MenuItem::Continue : MenuItem::NewGame);
}

MenuCommand Menu::handle(Key key, Settings& settings)
{
    const int count = static_cast<int>(m_items.size());
    switch (key) {
    case Key::Up:
        m_selected = (m_selected + count - 1) % count; // wraps from the top to the bottom
        return MenuCommand::None;
    case Key::Down:
        m_selected = (m_selected + 1) % count;
        return MenuCommand::None;
    case Key::Left:
        return change(m_items[static_cast<std::size_t>(m_selected)], -1, settings);
    case Key::Right:
        return change(m_items[static_cast<std::size_t>(m_selected)], +1, settings);
    case Key::Enter:
        return activate(m_items[static_cast<std::size_t>(m_selected)], settings);
    case Key::Escape:
        if (m_onSettingsPage) {
            showMainPage(MenuItem::Settings);
            return MenuCommand::None;
        }
        return m_canContinue ? MenuCommand::Continue : MenuCommand::None;
    default:
        return MenuCommand::None;
    }
}

void Menu::showMainPage(MenuItem selected)
{
    m_onSettingsPage = false;
    if (m_canContinue) {
        m_items = {MenuItem::Continue, MenuItem::NewGame, MenuItem::Settings, MenuItem::Quit};
    } else {
        m_items = {MenuItem::NewGame, MenuItem::Settings, MenuItem::Quit};
    }
    const auto found = std::ranges::find(m_items, selected);
    m_selected = found == m_items.end() ? 0 : static_cast<int>(found - m_items.begin());
}

void Menu::showSettingsPage()
{
    m_onSettingsPage = true;
    m_items = {MenuItem::Fullscreen, MenuItem::Speed, MenuItem::Walls, MenuItem::Sound, MenuItem::Back};
    m_selected = 0;
}

// Enter on an item.
MenuCommand Menu::activate(MenuItem item, Settings& settings)
{
    switch (item) {
    case MenuItem::Continue: return MenuCommand::Continue;
    case MenuItem::NewGame:  return MenuCommand::NewGame;
    case MenuItem::Quit:     return MenuCommand::Quit;
    case MenuItem::Settings:
        showSettingsPage();
        return MenuCommand::None;
    case MenuItem::Back:
        showMainPage(MenuItem::Settings);
        return MenuCommand::None;
    case MenuItem::Fullscreen:
    case MenuItem::Speed:
    case MenuItem::Walls:
    case MenuItem::Sound:
        return change(item, +1, settings); // Enter steps a setting forward, like Right
    }
    return MenuCommand::None;
}

// Left (-1) or Right (+1) on an item. Only settings react.
MenuCommand Menu::change(MenuItem item, int step, Settings& settings)
{
    switch (item) {
    case MenuItem::Fullscreen:
        settings.fullscreen = !settings.fullscreen; // on/off: either direction flips it
        return MenuCommand::SettingsChanged;
    case MenuItem::Speed:
        settings.speed = _next(settings.speed, step, kSpeedCount);
        return MenuCommand::SettingsChanged;
    case MenuItem::Walls: // two choices: either direction flips it
        settings.walls = settings.walls == game::Walls::Wrap ? game::Walls::Solid : game::Walls::Wrap;
        return MenuCommand::SettingsChanged;
    case MenuItem::Sound:
        settings.volume = _next(settings.volume, step, kVolumeCount);
        return MenuCommand::SettingsChanged;
    default:
        return MenuCommand::None;
    }
}

} // namespace snake::app
