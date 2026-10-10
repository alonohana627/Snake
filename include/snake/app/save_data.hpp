#pragma once

#include "snake/app/settings.hpp"

#include <filesystem>

namespace snake::app {

// Everything the game remembers between runs. Stored as an INI file, e.g.:
//
//     ; Snake - settings and best score.
//     version = 1
//
//     [settings]
//     fullscreen = false
//     speed = normal
//     walls = wrap
//     sound = medium
//
//     [score]
//     best = 42
struct SaveData {
    Settings settings;
    int bestScore = 0;

    friend bool operator==(const SaveData&, const SaveData&) = default;
};

// Where the file lives: snake.ini in the user's data folder (on Windows
// %APPDATA%\Snake). The SNAKE_DATA_DIR environment variable picks another folder
// (handy for testing without touching the real file).
std::filesystem::path saveFilePath();

// Reads the file. A missing file, a missing key or a value that makes no sense
// (e.g. "speed = warp") gives the default for that value - it never fails.
SaveData loadSaveData(const std::filesystem::path& path);

// Writes the file. Returns false (after printing why) if it couldn't;
// the game keeps going either way.
bool saveSaveData(const std::filesystem::path& path, const SaveData& data);

} // namespace snake::app
