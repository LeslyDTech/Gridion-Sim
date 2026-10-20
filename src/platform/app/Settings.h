#pragma once

#include <filesystem>
#include <string>
#include <string_view>

namespace gridiron {

// User settings, saved as settings.json next to the executable.
struct Settings {
    float masterVolume = 0.8f; // 0..1
    bool fullscreen = false;
    bool showDebugOnStart = false;
    bool reducedEffects = false; // turns off screen shake

    bool operator==(const Settings& other) const {
        return masterVolume == other.masterVolume && fullscreen == other.fullscreen &&
               showDebugOnStart == other.showDebugOnStart && reducedEffects == other.reducedEffects;
    }
};

std::string settingsToJson(const Settings& settings);

// Never fails: missing or malformed fields keep their defaults, and the volume is clamped to 0..1,
// so a hand-edited or old settings file can't break startup.
Settings settingsFromJson(std::string_view json);

// Returns false if the file couldn't be written.
bool saveSettings(const Settings& settings, const std::filesystem::path& path);
// Defaults if the file is missing or unreadable.
Settings loadSettings(const std::filesystem::path& path);

} // namespace gridiron
