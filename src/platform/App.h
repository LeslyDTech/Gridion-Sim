#pragma once

#include "DebugOverlay.h"
#include "FollowCamera.h"
#include "InputDevice.h"
#include "core/FixedTimestep.h"

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

// Owns the window, the input device and the camera, and runs the main loop:
//   poll input -> run N fixed 60 Hz simulation steps -> render once, interpolated by alpha.
class App {
public:
    App();
    void run();

private:
    void runSimulationSteps(int steps);
    void handleAppCommands();
    void render(float alpha, float frameSeconds);

    RaylibWindow m_window;
    InputDevice m_input;
    FixedTimestep m_timestep;
    FollowCamera m_camera;
    DebugOverlay m_overlay;
    FrameStats m_stats;
    int m_fpsCapIndex = 0;

    // Temporary for step 2: a marker steered with the arrow keys, so the camera and
    // interpolation can be seen before players exist. Replaced by Game in a later step.
    Vec2 m_marker;
    Vec2 m_previousMarker;
};

} // namespace gridiron
