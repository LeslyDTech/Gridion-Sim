#pragma once

#include "app/Settings.h"

#include <filesystem>

namespace gridiron {

// Where settings persist between runs:
//   desktop  settings.json at `path` (next to the executable)
//   web      the browser's localStorage ("gridiron-settings"); `path` is unused. A file written
//            in the browser build would live in Emscripten's in-memory file system and vanish
//            when the tab closes.
// Either way, anything missing or unreadable gives the defaults (see app/Settings.h).
Settings loadStoredSettings(const std::filesystem::path& path);
void storeSettings(const Settings& settings, const std::filesystem::path& path);

} // namespace gridiron
