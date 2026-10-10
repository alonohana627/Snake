#include "snake/storage/files.hpp"

#include <cstdlib>
#include <fstream>
#include <iterator>
#include <system_error>

namespace snake::storage {

std::filesystem::path userDataDirectory(std::string_view appName)
{
#if defined(_WIN32)
    // The wide (UTF-16) version: a user name like "Łukasz" would break a narrow string.
    if (const wchar_t* const appData = _wgetenv(L"APPDATA")) {
        return std::filesystem::path(appData) / appName;
    }
#elif defined(__APPLE__)
    if (const char* const home = std::getenv("HOME")) {
        return std::filesystem::path(home) / "Library" / "Application Support" / appName;
    }
#else
    if (const char* const dataHome = std::getenv("XDG_DATA_HOME"); dataHome != nullptr && *dataHome != '\0') {
        return std::filesystem::path(dataHome) / appName;
    }
    if (const char* const home = std::getenv("HOME")) {
        return std::filesystem::path(home) / ".local" / "share" / appName;
    }
#endif
    // Last resort, so the game still runs (and saves somewhere) on an odd setup.
    return std::filesystem::temp_directory_path() / appName;
}

std::optional<std::string> readFile(const std::filesystem::path& path)
{
    // Binary: read the bytes as they are (text mode on Windows would turn \r\n into \n).
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        return std::nullopt;
    }
    return std::string(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
}

bool writeFileAtomically(const std::filesystem::path& path, std::string_view contents, std::string& error)
{
    std::error_code code;
    std::filesystem::create_directories(path.parent_path(), code);
    if (code) {
        error = "can't create " + path.parent_path().string() + ": " + code.message();
        return false;
    }

    std::filesystem::path temporary = path;
    temporary += ".tmp";
    {
        std::ofstream file(temporary, std::ios::binary | std::ios::trunc);
        file.write(contents.data(), static_cast<std::streamsize>(contents.size()));
        file.flush(); // hands the bytes to the OS. (A power cut could still lose them;
                      // fine for a game's settings - a real save system would also fsync.)
        if (!file) {
            error = "can't write " + temporary.string();
            return false;
        }
    } // closes the file before renaming it

    std::filesystem::rename(temporary, path, code); // replaces the old file if there is one
    if (code) {
        error = "can't replace " + path.string() + ": " + code.message();
        std::filesystem::remove(temporary, code);
        return false;
    }
    return true;
}

} // namespace snake::storage
