#include "snake/app/save_data.hpp"

#include "snake/storage/files.hpp"
#include "snake/storage/ini.hpp"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <optional>
#include <print>
#include <span>
#include <string>
#include <string_view>

namespace snake::app {

namespace {

constexpr std::string_view kFileName = "snake.ini";
constexpr std::string_view kHeader = "Snake - settings and best score.\n"
                                     "Edit only while the game is closed: it rewrites this file.";

// Bump when a value's meaning changes, so a newer game can convert old files.
// (Adding a key doesn't need it: missing keys just get their default.)
constexpr int kVersion = 1;

// How each setting's values are spelled in the file.
template <typename Enum>
struct Name {
    Enum value;
    std::string_view text;
};

constexpr Name<Speed> kSpeedNames[] = {
    {Speed::Slow, "slow"}, {Speed::Normal, "normal"}, {Speed::Fast, "fast"}};
constexpr Name<game::Walls> kWallsNames[] = {
    {game::Walls::Wrap, "wrap"}, {game::Walls::Solid, "solid"}};
constexpr Name<Volume> kVolumeNames[] = {
    {Volume::Off, "off"}, {Volume::Low, "low"}, {Volume::Medium, "medium"}, {Volume::High, "high"}};
constexpr Name<bool> kBoolNames[] = {
    {false, "false"}, {true, "true"}};

} // namespace

namespace { // helpers

// Case doesn't matter when reading ("Fast" = "fast"): people edit this file by hand.
bool _equalsIgnoringCase(std::string_view a, std::string_view b)
{
    return std::ranges::equal(a, b, [](char x, char y) {
        return std::tolower(static_cast<unsigned char>(x)) == std::tolower(static_cast<unsigned char>(y));
    });
}

template <typename Enum>
std::string _toText(Enum value, std::span<const Name<Enum>> names)
{
    const auto found = std::ranges::find(names, value, &Name<Enum>::value);
    return found == names.end() ? std::string() : std::string(found->text);
}

// The value spelled `text`, or `fallback` if the key is missing or `text` isn't a known spelling.
template <typename Enum>
Enum _fromText(std::optional<std::string_view> text, std::span<const Name<Enum>> names, Enum fallback)
{
    if (!text) {
        return fallback;
    }
    const auto found = std::ranges::find_if(names, [&](const Name<Enum>& name) {
        return _equalsIgnoringCase(name.text, *text);
    });
    return found == names.end() ? fallback : found->value;
}

// A whole non-negative number, or `fallback`. from_chars, unlike std::stoi, never
// throws and doesn't depend on the system's language settings.
int _toCount(std::optional<std::string_view> text, int fallback)
{
    int value = 0;
    if (!text) {
        return fallback;
    }
    const auto [end, error] = std::from_chars(text->data(), text->data() + text->size(), value);
    if (error != std::errc{} || end != text->data() + text->size() || value < 0) {
        return fallback;
    }
    return value;
}

} // namespace

std::filesystem::path saveFilePath()
{
    if (const char* const directory = std::getenv("SNAKE_DATA_DIR"); directory != nullptr && *directory != '\0') {
        return std::filesystem::path(directory) / kFileName;
    }
    return storage::userDataDirectory("Snake") / kFileName;
}

SaveData loadSaveData(const std::filesystem::path& path)
{
    const SaveData defaults;
    const std::optional<std::string> text = storage::readFile(path);
    if (!text) {
        return defaults; // first run: no file yet
    }
    const storage::Ini ini = storage::Ini::parse(*text);

    // Only version 1 exists so far. A future version 2 would convert old values here.
    SaveData data;
    Settings& settings = data.settings;
    settings.fullscreen = _fromText<bool>(ini.get("settings", "fullscreen"), kBoolNames, defaults.settings.fullscreen);
    settings.speed = _fromText<Speed>(ini.get("settings", "speed"), kSpeedNames, defaults.settings.speed);
    settings.walls = _fromText<game::Walls>(ini.get("settings", "walls"), kWallsNames, defaults.settings.walls);
    settings.volume = _fromText<Volume>(ini.get("settings", "sound"), kVolumeNames, defaults.settings.volume);
    data.bestScore = _toCount(ini.get("score", "best"), defaults.bestScore);
    return data;
}

bool saveSaveData(const std::filesystem::path& path, const SaveData& data)
{
    storage::Ini ini;
    ini.set("", "version", std::to_string(kVersion));
    ini.set("settings", "fullscreen", _toText<bool>(data.settings.fullscreen, kBoolNames));
    ini.set("settings", "speed", _toText<Speed>(data.settings.speed, kSpeedNames));
    ini.set("settings", "walls", _toText<game::Walls>(data.settings.walls, kWallsNames));
    ini.set("settings", "sound", _toText<Volume>(data.settings.volume, kVolumeNames));
    ini.set("score", "best", std::to_string(data.bestScore));

    std::string error;
    if (!storage::writeFileAtomically(path, ini.toString(kHeader), error)) {
        std::println(stderr, "Couldn't save settings: {}", error);
        return false;
    }
    return true;
}

} // namespace snake::app
