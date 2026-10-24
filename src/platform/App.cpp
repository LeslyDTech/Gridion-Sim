#include "App.h"

#include "AttractMode.h"
#include "LoadingScene.h"
#include "Theme.h"

#include <raylib.h>

#if defined(__EMSCRIPTEN__)
#include <emscripten.h>
#endif

#include <array>
#include <memory>

namespace gridiron {
namespace {

constexpr int kWindowWidth = 1280;
constexpr int kWindowHeight = 720;

// F3 cycles through these render caps to show that simulation speed does not depend on them.
// In the browser the page's refresh rate paces frames instead, so there is no cap (0) and F3 does
// nothing: raylib's frame limiter would sleep inside the browser's frame callback.
#if defined(__EMSCRIPTEN__)
constexpr std::array<int, 1> kFpsCaps = {0};
#else
constexpr std::array<int, 5> kFpsCaps = {144, 60, 30, 240, 0};
#endif

// data/, assets/ and settings.json live next to the executable (the build copies data/ and
// assets/ there; see src/platform/CMakeLists.txt).
std::filesystem::path applicationDirectory() {
    return std::filesystem::path(GetApplicationDirectory());
}

} // namespace

RaylibWindow::RaylibWindow(int width, int height, const char* title) {
    SetTraceLogLevel(LOG_WARNING); // keep the console quiet unless something is wrong
#if defined(__EMSCRIPTEN__)
    // A fixed 1280x720 canvas: the page lays out around it (web/shell.html).
    SetConfigFlags(FLAG_MSAA_4X_HINT);
#else
    SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);
#endif
    InitWindow(width, height, title);
    SetExitKey(KEY_NULL); // Esc opens the pause menu; it must never silently quit
}

RaylibWindow::~RaylibWindow() { CloseWindow(); }

AudioDevice::AudioDevice() { InitAudioDevice(); }
AudioDevice::~AudioDevice() { CloseAudioDevice(); }

App::App()
    : m_window(kWindowWidth, kWindowHeight, "Gridiron Sim"),
      m_settingsPath(applicationDirectory() / "settings.json"),
      m_settings(loadStoredSettings(m_settingsPath)),
      m_context{m_assets, m_input, m_scenes, m_overlay, m_settings, m_settingsPath} {
    SetTargetFPS(kFpsCaps[0]);
    m_context.fpsCap = kFpsCaps[0];
    applySettings();
    if (m_settings.showDebugOnStart) {
        m_overlay.toggle();
    }

    // Only the loading screen's own fonts load up front; LoadingScene streams in the rest.
    m_assets.loadBootFonts(applicationDirectory() / "assets");
    m_scenes.reset(std::make_unique<LoadingScene>(m_context), true);
}

App::~App() {
    // Scenes hold references into m_context, which is destroyed before m_scenes: tear the
    // scenes down first, while everything they reference still exists.
    m_scenes.clear();
}

void App::applySettings() {
    SetMasterVolume(m_settings.masterVolume);
#if !defined(__EMSCRIPTEN__)
    // Browsers only allow fullscreen in response to a click or key press, never at page load.
    if (m_settings.fullscreen != IsWindowState(FLAG_BORDERLESS_WINDOWED_MODE)) {
        ToggleBorderlessWindowed();
    }
#endif
}

void App::run() {
#if defined(__EMSCRIPTEN__)
    // fps 0: use the browser's requestAnimationFrame. `true`: don't return to main(), whose stack
    // frame is gone once the browser takes over (main.cpp keeps App on the heap for this reason).
    emscripten_set_main_loop_arg([](void* app) { static_cast<App*>(app)->frame(); }, this, 0, true);
#else
    while (!WindowShouldClose() && !m_context.quitRequested) {
        frame();
    }
#endif
}

void App::frame() {
    const float frameSeconds = GetFrameTime();
    m_input.poll();
    handleAppCommands();
    m_scenes.update(frameSeconds);
    m_context.frame.frameSeconds = frameSeconds;
    m_overlay.record(m_context.frame);
    drawFrame();
}

void App::handleAppCommands() {
    const AppCommands& commands = m_input.appCommands();
    if (commands.toggleDebug) {
        m_overlay.toggle();
    }
    if (commands.togglePolish) {
        m_context.polish = !m_context.polish;
    }
    if (commands.cycleFpsCap) {
        m_fpsCapIndex = (m_fpsCapIndex + 1) % static_cast<int>(kFpsCaps.size());
        m_context.fpsCap = kFpsCaps[static_cast<std::size_t>(m_fpsCapIndex)];
        SetTargetFPS(m_context.fpsCap);
    }
}

void App::drawFrame() {
    BeginDrawing();
    ClearBackground(theme::kUiDark);
    m_scenes.draw();
    m_overlay.drawStats(m_assets,
                        OverlaySettings{m_context.fpsCap, m_context.polish, m_context.playerCount});

    // Scene transition: fade through black.
    const float fade = m_scenes.fadeAlpha();
    if (fade > 0.0f) {
        DrawRectangle(0, 0, GetScreenWidth(), GetScreenHeight(),
                      Fade(theme::menu::kSceneFade, fade));
    }
    EndDrawing();
}

} // namespace gridiron
