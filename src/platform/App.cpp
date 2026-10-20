#include "App.h"

#include "Renderer.h"
#include "core/PlayLoader.h"

#include <raylib.h>

#include <array>
#include <chrono>
#include <filesystem>

namespace gridiron {
namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
constexpr double kSimulationHz = 60.0;
constexpr float kPixelsPerYard = 16.0f; // 720 px tall window shows 45 of the 53 yards of width

// F2 cycles through these render caps to show that simulation speed does not depend on them.
constexpr std::array<int, 5> kFpsCaps = {144, 60, 30, 240, 0};

// Exponential moving average: smooths the update-time readout so it is readable.
double smooth(double average, double sample) { return average + (sample - average) * 0.05; }

// data/ is copied next to the executable by the build (see src/platform/CMakeLists.txt).
std::filesystem::path dataDirectory() {
    return std::filesystem::path(GetApplicationDirectory()) / "data";
}

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
      m_camera(kPixelsPerYard) {
    SetTargetFPS(kFpsCaps[0]);
    loadGame();
}

void App::loadGame() {
    Playbook offense = loadPlaysFromDirectory(dataDirectory() / "plays");
    Playbook defense = loadPlaysFromDirectory(dataDirectory() / "defense");

    for (const std::string& error : offense.errors) {
        TraceLog(LOG_WARNING, "Play skipped: %s", error.c_str());
    }
    if (offense.plays.empty() || defense.plays.empty()) {
        m_loadError = "Could not load plays from " + dataDirectory().string();
        if (!offense.errors.empty()) {
            m_loadError += "\n" + offense.errors.front();
        }
        if (!defense.errors.empty()) {
            m_loadError += "\n" + defense.errors.front();
        }
        return;
    }
    m_game.emplace(std::move(offense.plays), std::move(defense.plays.front()));
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
    m_stats.stepsThisFrame = steps;
    if (!m_game || steps == 0) {
        return;
    }

    using Clock = std::chrono::steady_clock;
    const auto start = Clock::now();
    const float dt = static_cast<float>(m_timestep.stepSeconds());
    for (int i = 0; i < steps; ++i) {
        m_game->step(m_input.consumeForStep(), dt);
        if (m_game->playJustStarted()) {
            const Vec2 spot = m_game->world().ball().position;
            m_camera.snapTo(spot, GetScreenWidth(), GetScreenHeight());
        }
    }
    const std::chrono::duration<double, std::milli> elapsed = Clock::now() - start;
    m_stats.updateMs = smooth(m_stats.updateMs, elapsed.count() / steps);
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
    if (!m_game) {
        renderLoadError();
        return;
    }
    const World& world = m_game->world();

    // Follow the ball where it is *drawn* (interpolated), so the camera never jitters
    // against the players.
    const Ball& ball = world.ball();
    m_camera.follow(lerp(ball.previousPosition, ball.position, alpha), frameSeconds,
                    GetScreenWidth(), GetScreenHeight());

    BeginDrawing();
    ClearBackground(BLACK);

    BeginMode2D(m_camera.raylibCamera());
    render::drawFieldSurface();
    render::drawLineOfScrimmage(world);
    render::drawRoutes(world);
    render::drawPlayers(world, alpha);
    render::drawBall(world, alpha);
    EndMode2D();

    render::drawFieldNumbers(m_camera);
    render::drawPlayerLabels(world, alpha, m_camera, m_overlay.enabled());
    render::drawHud(*m_game);

    m_stats.fps = GetFPS();
    m_stats.fpsCap = kFpsCaps[m_fpsCapIndex];
    m_overlay.drawStats(m_stats);
    EndDrawing();
}

void App::renderLoadError() const {
    BeginDrawing();
    ClearBackground(Color{40, 10, 10, 255});
    DrawText("Failed to load play data", 40, 40, 32, RAYWHITE);
    DrawText(m_loadError.c_str(), 40, 100, 20, LIGHTGRAY);
    EndDrawing();
}

} // namespace gridiron
