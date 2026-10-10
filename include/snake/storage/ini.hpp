#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace snake::storage {

// A minimal INI file: "key = value" lines, grouped under "[section]" headers.
// Lines starting with ; or # are comments. Keys before the first header belong
// to the section "" (empty name). Example:
//
//     version = 1
//
//     [settings]
//     fullscreen = true   (no comments at the end of a line, though)
//
// Reading is forgiving: lines it doesn't understand are skipped, so a hand-edited
// file with a typo loses that one line, not everything.
class Ini {
public:
    static Ini parse(std::string_view text);

    // The value, if the file has that key. Whitespace around it is already trimmed.
    std::optional<std::string_view> get(std::string_view section, std::string_view key) const;

    // Adds the key, or changes it if it's already there.
    void set(std::string_view section, std::string_view key, std::string value);

    // Back to text: `header` as comment lines at the top, then the keys of each
    // section, in the order they were first set.
    std::string toString(std::string_view header = {}) const;

private:
    struct Entry {
        std::string section;
        std::string key;
        std::string value;
    };

    std::vector<Entry> m_entries; // a handful of keys: a plain list is simplest
};

} // namespace snake::storage
