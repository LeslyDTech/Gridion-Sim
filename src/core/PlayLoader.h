#pragma once

#include "PlayDefinition.h"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace gridiron {

// Either a value or a human-readable error. (C++23 has std::expected for this; this is the
// minimal C++17 version.)
template <typename T>
struct ParseResult {
    std::optional<T> value;
    std::string error;

    bool ok() const { return value.has_value(); }
};

// Parses and validates one play from JSON text. Never throws. Errors name the offending field,
// e.g. "players[3].route: required for assignment 'route'".
ParseResult<PlayDefinition> parsePlay(std::string_view jsonText);

// Reads a file and parses it. The error message is prefixed with the file name.
ParseResult<PlayDefinition> loadPlayFile(const std::filesystem::path& path);

struct Playbook {
    std::vector<PlayDefinition> plays;
    std::vector<std::string> errors; // one entry per file that failed to load
};

// Loads every *.json file in a directory, sorted by file name so the order is deterministic.
// A bad file is reported in `errors` and skipped; it doesn't prevent the others from loading.
Playbook loadPlaysFromDirectory(const std::filesystem::path& directory);

} // namespace gridiron
