#include "LoadingScene.h"

#include "Assets.h"
#include "GameScene.h"
#include "InputDevice.h"
#include "MenuWidgets.h"
#include "Theme.h"
#include "UiDraw.h"
#include "app/SceneManager.h"

#include <raylib.h>

#include <array>
#include <cmath>
#include <cstdio>
#include <memory>

namespace gridiron {
namespace {

using namespace theme::menu;

// Installed per frame: enough to finish quickly, few enough to keep the screen animating.
constexpr int kUploadsPerFrame = 3;
constexpr std::size_t kMaxErrorLines = 6;

constexpr std::array<const char*, 6> kTips = {
    "TIP: Lead your receiver on slants",
    "TIP: Press 1-4 before the snap to pick a play",
    "TIP: Throw before the pass rush sheds its blocks",
    "TIP: Head-on tackles stick; runners at full speed break them",
    "TIP: Find the hole fast on the HB Dive",
    "TIP: Scroll to zoom, middle-click to reset the camera",
};

} // namespace

LoadingScene::LoadingScene(AppContext& context)
    : m_context(context),
      m_loader(std::filesystem::path(GetApplicationDirectory()), defaultManifest()) {}

void LoadingScene::enter() { m_loader.start(); }

void LoadingScene::update(float frameSeconds) {
    m_elapsed += frameSeconds;
    m_loader.pump(m_context.assets, m_context, kUploadsPerFrame);
    m_shownProgress +=
        (m_loader.progress() - m_shownProgress) * (1.0f - std::exp(-12.0f * frameSeconds));

    const MenuCommands& input = m_context.input.menuCommands();
    if (m_loader.fatal()) {
        if (input.back) {
            m_context.quitRequested = true;
        }
        return;
    }
    const bool ready = m_loader.finished() && m_elapsed >= kLoadingMinimumSeconds;
    if (!ready) {
        return;
    }
    if (m_loader.errors().empty() || input.confirm) {
        proceed();
    }
}

void LoadingScene::proceed() {
    if (m_proceeding) {
        return;
    }
    m_proceeding = true;
    m_context.scenes.replace(std::make_unique<GameScene>(m_context, GameMode::Drive));
}

void LoadingScene::draw() {
    const Assets& assets = m_context.assets;
    menu::drawFieldPlaceholder();
    menu::drawBackdrop(assets);
    menu::drawBrandCentered(assets, ui::centeredTop(kLoadingBrandTop),
                            ui::centeredTop(kLoadingBrandUnderlineTop),
                            kLoadingBrandUnderlineWidth);

    // "Asset loading progress"
    const float left = std::round((ui::screenWidth() - kAssetLoadingProgressWidth) / 2.0f);
    const float top = ui::centeredTop(kAssetLoadingProgressTop);
    DrawRectangleRec(Rectangle{left, top, kAssetLoadingProgressWidth, kProgressTrackHeight},
                     kProgressTrack);
    DrawRectangleRec(
        Rectangle{left, top, kAssetLoadingProgressWidth * m_shownProgress, kProgressTrackHeight},
        kProgressFill);

    // "Loading status": percent on the left, current file on the right.
    char percent[8];
    std::snprintf(percent, sizeof(percent), "%d%%",
                  static_cast<int>(std::lround(m_shownProgress * 100.0f)));
    const float statusTop = top + kLoadingStatusOffsetY;
    ui::drawText(assets, kLoadingStatus, percent, Vector2{left, statusTop}, kLoadingStatus.color);
    char current[192];
    if (m_loader.finished()) {
        std::snprintf(current, sizeof(current), "Loaded %zu of %zu assets",
                      m_loader.total() - m_loader.errors().size(), m_loader.total());
    } else {
        std::snprintf(current, sizeof(current), "Loading %s", m_loader.currentPath().c_str());
    }
    const Vector2 currentSize = ui::measure(assets, kLoadingStatus, current);
    ui::drawText(assets, kLoadingStatus, current,
                 Vector2{left + kAssetLoadingProgressWidth - currentSize.x, statusTop},
                 kLoadingStatus.color);

    // Missing files, listed instead of crashing.
    const auto& errors = m_loader.errors();
    float errorTop = statusTop + ui::pixelSize(kLoadingStatus) + 12.0f;
    for (std::size_t i = 0; i < errors.size() && i < kMaxErrorLines; ++i) {
        char line[256];
        std::snprintf(line, sizeof(line), "Could not load %s", errors[i].c_str());
        ui::drawText(assets, kLoadingError, line, Vector2{left, errorTop}, kLoadingError.color);
        errorTop += ui::pixelSize(kLoadingError) + 2.0f;
    }
    if (!errors.empty() && m_loader.finished()) {
        const char* hint = m_loader.fatal() ? "Play data is required. Press ESC to quit."
                                            : "Press ENTER to continue without these files.";
        ui::drawText(assets, kLoadingStatus, hint, Vector2{left, errorTop + 6.0f},
                     kLoadingStatus.color);
    }

    // "Gameplay tip pill", rotating.
    const auto tipIndex = static_cast<std::size_t>(m_elapsed / kTipSeconds) % kTips.size();
    const char* tip = kTips[tipIndex];
    const Vector2 tipSize = ui::measure(assets, kReceiverTip, tip);
    const float pillWidth = tipSize.x + 2.0f * kGameplayTipPillPaddingX;
    const Rectangle pill{std::round((ui::screenWidth() - pillWidth) / 2.0f),
                         ui::bottomAnchoredTop(kGameplayTipPillTop), pillWidth,
                         kGameplayTipPillHeight};
    ui::fillRounded(pill, kGameplayTipPillCornerRadius, kGameplayTipPillBackground);
    ui::drawTextInBox(assets, kReceiverTip, tip, pill, kGameplayTipPillPaddingX, 0.5f);
}

} // namespace gridiron
