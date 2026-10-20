#include "App.h"

#include "Renderer.h"
#include "core/FieldGeometry.h"

#include <raylib.h>

#include <array>
#include <chrono>

namespace gridiron {
namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
constexpr double kSimulationHz = 60.0;
constexpr float kPixelsPerYard = 20.0f;

// F2 cycles through these render caps to show that simulation speed does not depend on them.
constexpr std::array<int, 5> kFpsCaps = {144, 60, 30, 240, 0};

// Exponential moving average: smooths the update-time readout so it is readable.
double smooth(double average, double sample) { return average + (sample - average) * 0.05; }

} // namespace

RaylibWindow::RaylibWindow(int width, int height, const char* title) {
    SetTraceLogLevel(LOG_WARNING); // keep the console quiet unless something is wrong
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
    InitWindow(width, height, title);
    SetExitKey(KEY_NULL); // Esc should not silently quit mid-play
}

RaylibWindow::~RaylibWindow() { CloseWindow(); }

App::App()
    : m_window(kWindowWidth, kWindowHeight, "Gridiron Sim"), m_timestep(1.0 / kSimulationHz),
      m_camera(kPixelsPerYard),
      m_marker{field::kStartingLineOfScrimmageX, field::kCenterY}, m_previousMarker(m_marker) {
    SetTargetFPS(kFpsCaps[0]);
    m_camera.snapTo(m_marker, GetScreenWidth(), GetScreenHeight());
}

void App::run() {
    while (!WindowShouldClose()) {
        const float frameSeconds = GetFrameTime();
        m_input.poll();
        handleAppCommands();
        runSimulationSteps(m_timestep.advance(frameSeconds));
        render(static_cast<float>(m_timestep.alpha()), frameSeconds);
    }
}

void App::runSimulationSteps(int steps) {
    using Clock = std::chrono::steady_clock;
    const auto start = Clock::now();

    const float dt = static_cast<float>(m_timestep.stepSeconds());
    for (int i = 0; i < steps; ++i) {
        const InputCommands commands = m_input.consumeForStep();
        m_previousMarker = m_marker;
        m_marker += commands.move * (10.0f * dt); // 10 yards/second
    }

    m_stats.stepsThisFrame = steps;
    if (steps > 0) {
        const std::chrono::duration<double, std::milli> elapsed = Clock::now() - start;
        m_stats.updateMs = smooth(m_stats.updateMs, elapsed.count() / steps);
    }
}

void App::handleAppCommands() {
    const AppCommands& commands = m_input.appCommands();
    if (commands.toggleDebug) {
        m_overlay.toggle();
    }
    if (commands.cycleFpsCap) {
        m_fpsCapIndex = (m_fpsCapIndex + 1) % static_cast<int>(kFpsCaps.size());
        SetTargetFPS(kFpsCaps[m_fpsCapIndex]);
    }
}

void App::render(float alpha, float frameSeconds) {
    // Draw where the simulation was `alpha` of the way between its last two steps. Without
    // this, motion would stutter whenever the render rate isn't an exact multiple of 60 Hz.
    const Vec2 marker = lerp(m_previousMarker, m_marker, alpha);
    m_camera.follow(marker, frameSeconds, GetScreenWidth(), GetScreenHeight());

    BeginDrawing();
    ClearBackground(BLACK);

    BeginMode2D(m_camera.raylibCamera());
    render::drawFieldSurface();
    DrawCircleV(Vector2{marker.x, marker.y}, 0.4f, ORANGE);
    EndMode2D();

    render::drawFieldNumbers(m_camera);

    m_stats.fps = GetFPS();
    m_stats.fpsCap = kFpsCaps[m_fpsCapIndex];
    m_overlay.drawStats(m_stats);
    EndDrawing();
}

} // namespace gridiron
