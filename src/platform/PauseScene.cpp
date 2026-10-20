#include "PauseScene.h"

#include "Assets.h"
#include "BroadcastHud.h"
#include "GameScene.h"
#include "InputDevice.h"
#include "Theme.h"
#include "UiDraw.h"
#include "app/SceneManager.h"

#include <array>
#include <memory>

namespace gridiron {
namespace {

using namespace theme::menu;

enum Option { kResume, kRestartDrive, kControls, kSettings, kMainMenu, kOptionCount };
constexpr std::array<const char*, kOptionCount> kLabels = {"RESUME", "RESTART DRIVE", "CONTROLS",
                                                           "SETTINGS", "MAIN MENU"};

Rectangle panelRect() {
    return {ui::screenWidth() / 2.0f - kPauseMenuPanelSize / 2.0f,
            ui::screenHeight() / 2.0f - kPauseMenuPanelSize / 2.0f + kPauseMenuPanelCenterOffsetY,
            kPauseMenuPanelSize, kPauseMenuPanelSize};
}

} // namespace

PauseScene::PauseScene(AppContext& context, GameScene& game)
    : m_context(context), m_game(game), m_selection(kOptionCount) {}

menu::ListLayout PauseScene::optionsLayout() const {
    const Rectangle panel = panelRect();
    return {panel.x + kPauseMenuOptionsLeft, panel.y + kPauseMenuOptionsTop,
            kPauseMenuOptionsWidth};
}

void PauseScene::update(float /*frameSeconds*/) {
    const MenuCommands& input = m_context.input.menuCommands();
    if (input.back || input.pause) {
        m_context.scenes.pop(); // resume
        return;
    }
    const int chosen = m_selection.update(input, optionsLayout());
    if (chosen >= 0) {
        activate(chosen);
    }
}

void PauseScene::activate(int option) {
    switch (option) {
    case kResume:
        m_context.scenes.pop();
        break;
    case kRestartDrive:
        m_game.game().restartDrive();
        m_context.scenes.pop();
        break;
    case kControls:
    case kSettings:
        // Wired up together with the Controls and Settings scenes.
        break;
    case kMainMenu:
        // Until the main menu exists: start a fresh drive.
        m_context.scenes.reset(std::make_unique<GameScene>(m_context, m_game.game().mode()));
        break;
    default:
        break;
    }
}

void PauseScene::draw() {
    const Assets& assets = m_context.assets;
    menu::drawBackdrop(assets);

    // "Pause menu panel"
    const Rectangle panel = panelRect();
    ui::fillRounded(panel, kPauseMenuPanelCornerRadius, kPauseMenuPanelBackground);
    const Vector2 titleSize = ui::measure(assets, kPausedTitle, kPausedTitleText);
    ui::drawText(
        assets, kPausedTitle, kPausedTitleText,
        Vector2{panel.x + (panel.width - titleSize.x) / 2.0f,
                ui::glyphTop(panel.y + kPausedTitleTop, kPausedTitle, kPausedTitleLineHeight)},
        kPausedTitle.color);
    menu::drawOptions(assets, optionsLayout(), kLabels.data(), nullptr, kOptionCount,
                      m_selection.selected());

    // "Paused game scorebug": the real one, above the darkening.
    render::broadcast::drawScorebug(m_game.game(), assets);
}

} // namespace gridiron
