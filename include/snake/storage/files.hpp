#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace snake::storage {

// Where a program should keep the current user's files, in a folder named `appName`:
//   Windows: %APPDATA%\<appName>        (e.g. C:\Users\you\AppData\Roaming\Snake)
//   macOS:   ~/Library/Application Support/<appName>
//   Linux:   $XDG_DATA_HOME/<appName>   (usually ~/.local/share/<appName>)
// Never next to the .exe: that folder is often read-only (e.g. Program Files).
// The folder may not exist yet - writeFileAtomically creates it.
std::filesystem::path userDataDirectory(std::string_view appName);

// The whole file as text, or nothing if it doesn't exist or can't be read.
std::optional<std::string> readFile(const std::filesystem::path& path);

// Replaces the file's contents "atomically": writes everything to a temporary file
// next to it, then renames that over the real one. Renaming is all-or-nothing, so
// if the game crashes mid-write, the old file is still intact - never half a file.
// Creates missing folders. Returns false (and the reason in `error`) if it failed.
bool writeFileAtomically(const std::filesystem::path& path, std::string_view contents, std::string& error);

} // namespace snake::storage
