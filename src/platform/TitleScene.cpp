#include "TitleScene.h"

#include "Assets.h"
#include "AttractMode.h"
#include "GameScene.h"
#include "InputDevice.h"
#include "MenuWidgets.h"
#include "Theme.h"
#include "UiDraw.h"
#include "app/SceneManager.h"

#include <raylib.h>

#include <cmath>
#include <memory>

namespace gridiron {
namespace {

using namespace theme::menu;

// Ignore input for a moment after the title appears, so the key that finished loading (Enter)
// doesn't skip straight past it.
constexpr float kInputDelaySeconds = 0.4f;

// Draws a single line of text centered horizontally at a Figma `top` (leading-none text).
void drawCenteredLine(const Assets& assets, const TextStyle& style, const char* text, float top,
                      float lineHeight, Color color) {
    const Vector2 size = ui::measure(assets, style, text);
    ui::drawText(assets, style, text,
                 Vector2{(ui::screenWidth() - size.x) / 2.0f, ui::glyphTop(top, style, lineHeight)},
                 color);
}

} // namespace

void TitleScene::update(float frameSeconds) {
    m_time += frameSeconds;
    if (m_context.attract != nullptr) {
        m_context.attract->update(frameSeconds);
    }
    if (m_time >= kInputDelaySeconds && m_context.input.menuCommands().anyKey) {
        m_context.scenes.replace(std::make_unique<GameScene>(m_context, GameMode::Drive));
    }
}

void TitleScene::draw() {
    const Assets& assets = m_context.assets;
    if (m_context.attract != nullptr) {
        m_context.attract->draw(assets);
    }
    menu::drawBackdrop(assets);

    menu::drawBrandCentered(assets, ui::centeredTop(kTitleBrandTop), 0.0f, 0.0f);
    drawCenteredLine(assets, kGameSubtitle, kGameSubtitleText, ui::centeredTop(kGameSubtitleTop),
                     kLineHeightNormal, kGameSubtitle.color);

    // One PRESS START line, fading between the bright (100%) and dim (40%) states each second.
    const float wave = 0.5f + 0.5f * std::cos(2.0f * PI * m_time / kPressStartPulseSeconds);
    const float opacity = kPressStartDimOpacity + (1.0f - kPressStartDimOpacity) * wave;
    drawCenteredLine(assets, kPressStart, kPressStartText, ui::centeredTop(kPressStartTop),
                     kLineHeightNone, Fade(kPressStart.color, opacity));

    // "Build version", bottom right.
    const Vector2 size = ui::measure(assets, kBuildVersion, kBuildVersionText);
    ui::drawText(assets, kBuildVersion, kBuildVersionText,
                 Vector2{ui::screenWidth() - kBuildVersionRight - size.x,
                         ui::screenHeight() - kBuildVersionBottom},
                 kBuildVersion.color);
}

} // namespace gridiron
