#include "Settings.h"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <type_traits>

namespace gridiron {
namespace {

using nlohmann::json;

template <typename T>
void readIf(const json& root, const char* key, T& out) {
    const auto it = root.find(key);
    if (it == root.end()) {
        return;
    }
    if constexpr (std::is_same_v<T, bool>) {
        if (it->is_boolean()) {
            out = it->template get<bool>();
        }
    } else {
        if (it->is_number()) {
            out = it->template get<T>();
        }
    }
}

} // namespace

std::string settingsToJson(const Settings& settings) {
    const json root = {
        {"masterVolume", settings.masterVolume},
        {"fullscreen", settings.fullscreen},
        {"showDebugOnStart", settings.showDebugOnStart},
        {"reducedEffects", settings.reducedEffects},
    };
    return root.dump(2);
}

Settings settingsFromJson(std::string_view text) {
    Settings settings;
    const json root = json::parse(text.begin(), text.end(), nullptr, false);
    if (root.is_discarded() || !root.is_object()) {
        return settings;
    }
    readIf(root, "masterVolume", settings.masterVolume);
    readIf(root, "fullscreen", settings.fullscreen);
    readIf(root, "showDebugOnStart", settings.showDebugOnStart);
    readIf(root, "reducedEffects", settings.reducedEffects);
    settings.masterVolume = std::clamp(settings.masterVolume, 0.0f, 1.0f);
    return settings;
}

bool saveSettings(const Settings& settings, const std::filesystem::path& path) {
    std::ofstream file(path);
    if (!file) {
        return false;
    }
    file << settingsToJson(settings) << '\n';
    return static_cast<bool>(file);
}

Settings loadSettings(const std::filesystem::path& path) {
    std::ifstream file(path);
    if (!file) {
        return Settings{};
    }
    std::ostringstream text;
    text << file.rdbuf();
    return settingsFromJson(text.str());
}

} // namespace gridiron
