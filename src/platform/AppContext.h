#pragma once

#include "DebugOverlay.h"
#include "app/Settings.h"
#include "core/PlayDefinition.h"

#include <filesystem>
#include <memory>
#include <vector>

namespace gridiron {

class Assets;
class AttractMode;
class InputDevice;
class SceneManager;

// The shared services every scene can use. Owned by App, which outlives every scene; scenes keep a
// reference. Holding these here (instead of globals) keeps scenes testable in principle and makes
// every dependency explicit.
struct AppContext {
    Assets& assets;
    InputDevice& input;
    SceneManager& scenes;
    DebugOverlay& overlay;
    Settings& settings;
    std::filesystem::path settingsPath;

    // Loaded by the loading scene (or synchronously at startup).
    std::vector<PlayDefinition> playbook;
    PlayDefinition defense;

    // Background play running behind the title and main menu (null until loading finishes).
    // AttractMode is only forward-declared here; App.cpp, which destroys the context, includes it.
    std::unique_ptr<AttractMode> attract;

    bool polish = true;  // F2
    int fpsCap = 144;    // F3
    FrameSample frame;   // filled in by GameScene for the debug overlay
    int playerCount = 0; // for the overlay's brute-force comparison
    bool quitRequested = false;

    // Writes settings.json next to the executable.
    void saveSettings() const { ::gridiron::saveSettings(settings, settingsPath); }
};

} // namespace gridiron
