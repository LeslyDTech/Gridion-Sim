#include "App.h"

#include "FieldRenderer.h"
#include "Hud.h"
#include "Renderer.h"
#include "Theme.h"
#include "core/PlayLoader.h"

#include <raylib.h>

#include <algorithm>
#include <array>
#include <chrono>
#include <filesystem>

namespace gridiron {
namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;
constexpr double kSimulationHz = 60.0;
constexpr float kVisibleYardsAcross = 32.0f; // close, broadcast-style framing on the ball

// F3 cycles through these render caps to show that simulation speed does not depend on them.
constexpr std::array<int, 5> kFpsCaps = {144, 60, 30, 240, 0};

// data/ and assets/ are copied next to the executable by the build
// (see src/platform/CMakeLists.txt).
std::filesystem::path dataDirectory() {
    return std::filesystem::path(GetApplicationDirectory()) / "data";
}
std::filesystem::path assetDirectory() {
    return std::filesystem::path(GetApplicationDirectory()) / "assets";
}

// Room around each player that must stay out from under the HUD: half a sprite, the jersey number
// below it and the receiver badge above it.
constexpr float kPlayerFramingMargin = 1.5f; // yards

// The box the camera must keep on screen: every player, where they are drawn this frame.
Bounds playerBounds(const World& world, float alpha) {
    Bounds bounds{{1.0e9f, 1.0e9f}, {-1.0e9f, -1.0e9f}};
    for (const Player& player : world.players()) {
        const Vec2 p = lerp(player.previousPosition, player.position, alpha);
        bounds.min = {std::min(bounds.min.x, p.x), std::min(bounds.min.y, p.y)};
        bounds.max = {std::max(bounds.max.x, p.x), std::max(bounds.max.y, p.y)};
    }
    const Vec2 margin{kPlayerFramingMargin, kPlayerFramingMargin};
    return {bounds.min - margin, bounds.max + margin};
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
    : m_window(kWindowWidth, kWindowHeight, "Gridiron Sim"), m_assets(assetDirectory()),
      m_timestep(1.0 / kSimulationHz), m_camera(kVisibleYardsAcross) {
    SetTargetFPS(kFpsCaps[0]);
    applyCameraSafeArea();
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
    m_frame.steps = steps;
    m_frame.updateSeconds = 0.0;
    if (!m_game || steps == 0) {
        return;
    }

    using Clock = std::chrono::steady_clock;
    const auto start = Clock::now();
    const float dt = static_cast<float>(m_timestep.stepSeconds());
    for (int i = 0; i < steps; ++i) {
        m_game->step(m_input.consumeForStep(), dt);
        if (m_game->playJustStarted()) {
            const World& world = m_game->world();
            m_camera.snapTo(world.ball().position, playerBounds(world, 1.0f), GetScreenWidth(),
                            GetScreenHeight());
        }
    }
    const std::chrono::duration<double> elapsed = Clock::now() - start;
    m_frame.updateSeconds = elapsed.count();
}

void App::applyCameraSafeArea() { m_camera.setSafeArea(render::hudSafeArea(m_polish)); }

void App::handleAppCommands() {
    const AppCommands& commands = m_input.appCommands();
    if (commands.toggleDebug) {
        m_overlay.toggle();
    }
    if (commands.togglePolish) {
        m_polish = !m_polish;
        applyCameraSafeArea();
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
    m_effects.update(world, frameSeconds);
    m_camera.follow(lerp(ball.previousPosition, ball.position, alpha), playerBounds(world, alpha),
                    frameSeconds, GetScreenWidth(), GetScreenHeight(),
                    m_polish ? m_effects.zoomFactor() : 1.0f,
                    m_polish ? m_effects.shakeOffset() : Vec2{});

    BeginDrawing();
    ClearBackground(theme::kUiDark);

    const RenderContext context{m_camera, m_assets, alpha, m_polish, GetTime()};

    BeginMode2D(m_camera.raylibCamera());
    render::drawField(context);
    render::drawLineOfScrimmage(world, context);
    if (world.phase() == PlayPhase::PreSnap) {
        render::drawFirstDownLine(m_game->drive().firstDownLineX(), context);
    }
    m_overlay.drawGrid(world.spatialHash());
    render::drawRoutes(world, context, m_effects.routeOpacity());
    if (m_polish) {
        m_effects.drawParticles();
    }
    render::drawPlayers(world, context, m_effects);
    render::drawBall(world, context);
    EndMode2D();

    render::drawVignette(context);
    render::drawPlayerLabels(world, context, m_overlay.enabled());
    m_hudAnimator.update(*m_game, frameSeconds);
    render::drawHud(*m_game, context, m_hudAnimator);

    m_frame.frameSeconds = frameSeconds;
    m_frame.collisionChecks = world.collisionChecks();
    m_overlay.record(m_frame);
    m_overlay.drawStats(m_assets, OverlaySettings{kFpsCaps[m_fpsCapIndex], m_polish,
                                                  static_cast<int>(world.players().size())});
    EndDrawing();
}

void App::renderLoadError() const {
    BeginDrawing();
    ClearBackground(theme::kUiDark);
    DrawText("Failed to load play data", 40, 40, 32, theme::kAway);
    DrawText(m_loadError.c_str(), 40, 100, 20, theme::kLineWhite);
    EndDrawing();
}

} // namespace gridiron
