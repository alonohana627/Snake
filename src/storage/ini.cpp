#include "snake/storage/ini.hpp"

#include <algorithm>
#include <ranges>

namespace snake::storage {

namespace { // helpers

std::string_view _trim(std::string_view text)
{
    constexpr std::string_view kWhitespace = " \t\r\n";
    const std::size_t first = text.find_first_not_of(kWhitespace);
    if (first == std::string_view::npos) {
        return {};
    }
    const std::size_t last = text.find_last_not_of(kWhitespace);
    return text.substr(first, last - first + 1);
}

} // namespace

Ini Ini::parse(std::string_view text)
{
    Ini ini;
    std::string section;

    for (const auto lineRange : text | std::views::split('\n')) {
        const std::string_view line = _trim(std::string_view(lineRange.begin(), lineRange.end()));

        if (line.empty() || line.front() == ';' || line.front() == '#') {
            continue;
        }
        if (line.front() == '[') {
            if (line.back() == ']') {
                section = _trim(line.substr(1, line.size() - 2));
            }
            continue;
        }
        if (const std::size_t equals = line.find('='); equals != std::string_view::npos) {
            const std::string_view key = _trim(line.substr(0, equals));
            if (!key.empty()) {
                ini.set(section, key, std::string(_trim(line.substr(equals + 1))));
            }
        }
        // Anything else isn't INI: skip it.
    }
    return ini;
}

std::optional<std::string_view> Ini::get(std::string_view section, std::string_view key) const
{
    const auto found = std::ranges::find_if(m_entries, [&](const Entry& entry) {
        return entry.section == section && entry.key == key;
    });
    if (found == m_entries.end()) {
        return std::nullopt;
    }
    return found->value;
}

void Ini::set(std::string_view section, std::string_view key, std::string value)
{
    const auto found = std::ranges::find_if(m_entries, [&](const Entry& entry) {
        return entry.section == section && entry.key == key;
    });
    if (found != m_entries.end()) {
        found->value = std::move(value);
    } else {
        m_entries.push_back({std::string(section), std::string(key), std::move(value)});
    }
}

std::string Ini::toString(std::string_view header) const
{
    std::string text;
    if (!header.empty()) {
        for (const auto lineRange : header | std::views::split('\n')) {
            text += "; ";
            text += std::string_view(lineRange.begin(), lineRange.end());
            text += '\n';
        }
    }

    // Sections in the order they first appear. "" (keys without a section) has no header,
    // so it must come first - otherwise its keys would land in the section above them.
    std::vector<std::string_view> sections{""};
    for (const Entry& entry : m_entries) {
        if (std::ranges::find(sections, entry.section) == sections.end()) {
            sections.push_back(entry.section);
        }
    }

    for (const std::string_view section : sections) {
        if (!section.empty()) {
            text += "\n[";
            text += section;
            text += "]\n";
        }
        for (const Entry& entry : m_entries) {
            if (entry.section == section) {
                text += entry.key + " = " + entry.value + '\n';
            }
        }
    }
    return text;
}

} // namespace snake::storage
