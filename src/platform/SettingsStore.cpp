#include "SettingsStore.h"

#if defined(__EMSCRIPTEN__)
#include <cstdlib>
#include <emscripten.h>
#include <string>

namespace {

// JavaScript helpers compiled into the page. localStorage can throw (private browsing, storage
// disabled), in which case settings just aren't remembered.
// clang-format off
EM_JS(char*, gridironReadSettings, (), {
    var json = null;
    try { json = window.localStorage.getItem("gridiron-settings"); } catch (e) {}
    return json === null ? 0 : stringToNewUTF8(json);
});

EM_JS(void, gridironWriteSettings, (const char* json), {
    try { window.localStorage.setItem("gridiron-settings", UTF8ToString(json)); } catch (e) {}
});
// clang-format on

} // namespace
#endif

namespace gridiron {

#if defined(__EMSCRIPTEN__)

Settings loadStoredSettings(const std::filesystem::path& /*path*/) {
    char* json = gridironReadSettings();
    if (json == nullptr) {
        return Settings{};
    }
    const Settings settings = settingsFromJson(json);
    std::free(json); // stringToNewUTF8 allocates with malloc
    return settings;
}

void storeSettings(const Settings& settings, const std::filesystem::path& /*path*/) {
    gridironWriteSettings(settingsToJson(settings).c_str());
}

#else

Settings loadStoredSettings(const std::filesystem::path& path) { return loadSettings(path); }

void storeSettings(const Settings& settings, const std::filesystem::path& path) {
    saveSettings(settings, path);
}

#endif

} // namespace gridiron
