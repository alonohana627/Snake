#pragma once

#include "snake/app/settings.hpp"
#include "snake/platform/window.hpp"

#include <span>
#include <vector>

namespace snake::app {

// One line in a menu.
enum class MenuItem {
    // Main page
    Continue, // back to the paused game (only shown when there is one)
    NewGame,
    Settings, // opens the settings page
    Quit,
    // Settings page
    Fullscreen,
    Speed,
    Sound, // sound volume
    Back,  // back to the main page
};

// What the application should do after the menu handled a key.
enum class MenuCommand {
    None,           // nothing (the menu only moved its selection or changed page)
    Continue,       // close the menu, keep playing the paused game
    NewGame,        // close the menu, start a new game
    Quit,           // close the window
    SettingsChanged // apply the (already changed) settings
};

// The menu's state and rules - which page, which items, which one is selected -
// with no drawing (menu_view does that). Up/Down move the selection, Enter
// picks, Left/Right change a setting, Escape goes back.
class Menu {
public:
    // Shows the main page. `canContinue`: a game is paused behind the menu.
    void open(bool canContinue);

    // Handles one key. Changes `settings` directly when a setting is changed.
    MenuCommand handle(platform::Key key, Settings& settings);

    std::span<const MenuItem> items() const { return m_items; }
    int selected() const { return m_selected; }
    bool onSettingsPage() const { return m_onSettingsPage; }

private:
    void showMainPage(MenuItem selected);
    void showSettingsPage();
    MenuCommand activate(MenuItem item, Settings& settings);
    MenuCommand change(MenuItem item, int step, Settings& settings);

    std::vector<MenuItem> m_items;
    int m_selected = 0; // index into m_items
    bool m_onSettingsPage = false;
    bool m_canContinue = false;
};

} // namespace snake::app
