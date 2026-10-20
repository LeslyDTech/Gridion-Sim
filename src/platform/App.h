#pragma once

#include "Assets.h"
#include "DebugOverlay.h"
#include "Effects.h"
#include "FollowCamera.h"
#include "InputDevice.h"
#include "core/FixedTimestep.h"
#include "core/Game.h"

#include <optional>
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

// Owns the window, the game and everything platform-specific, and runs the main loop:
//   poll input -> run N fixed 60 Hz simulation steps -> render once, interpolated by alpha.
class App {
public:
    App();
    void run();

private:
    void loadGame();
    void runSimulationSteps(int steps);
    void handleAppCommands();
    void applyCameraInset();
    void render(float alpha, float frameSeconds);
    void renderLoadError() const;

    RaylibWindow m_window; // first: everything below needs the window
    Assets m_assets;
    InputDevice m_input;
    FixedTimestep m_timestep;
    FollowCamera m_camera;
    DebugOverlay m_overlay;
    Effects m_effects;
    FrameSample m_frame; // measurements of the current frame, handed to the debug overlay
    int m_fpsCapIndex = 0;
    bool m_polish = true; // F2: all visual polish on/off, for before/after screenshots

    // Empty if the play files failed to load; the error is then shown on screen instead.
    std::optional<Game> m_game;
    std::string m_loadError;
};

} // namespace gridiron
