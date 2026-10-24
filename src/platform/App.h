#pragma once

#include "AppContext.h"
#include "Assets.h"
#include "DebugOverlay.h"
#include "InputDevice.h"
#include "app/SceneManager.h"
#include "app/Settings.h"

#include <filesystem>
#include <string>

namespace gridiron {

// RAII owner of the raylib window and OpenGL context. Declared as App's first member so it is
// created before, and destroyed after, everything that uses the window.
class RaylibWindow {
public:
    RaylibWindow(int width, int height, const char* title);
    ~RaylibWindow();
    RaylibWindow(const RaylibWindow&) = delete;
    RaylibWindow& operator=(const RaylibWindow&) = delete;
};

// RAII owner of the audio device (master volume lives here, set from Settings).
class AudioDevice {
public:
    AudioDevice();
    ~AudioDevice();
    AudioDevice(const AudioDevice&) = delete;
    AudioDevice& operator=(const AudioDevice&) = delete;
};

// The application shell: window, audio, settings, input, and the scene stack. Each frame:
//   poll input -> app keys (F1/F2/F3) -> update the top scene -> draw the stack -> fade.
// Everything game-specific lives in scenes (GameScene, menus...).
class App {
public:
    App();
    ~App();

    // Desktop: loops until the window closes or a scene asks to quit.
    // Browser: hands frame() to the browser, which calls it once per display refresh, and never
    // returns (a web page can't block in a loop; the tab would freeze).
    void run();

private:
    void frame();
    void handleAppCommands();
    void applySettings();
    void drawFrame();

    RaylibWindow m_window; // first: everything below needs the window
    AudioDevice m_audio;
    std::filesystem::path m_settingsPath;
    Settings m_settings;
    Assets m_assets;
    InputDevice m_input;
    DebugOverlay m_overlay;
    SceneManager m_scenes;
    AppContext m_context;
    int m_fpsCapIndex = 0;
};

} // namespace gridiron
