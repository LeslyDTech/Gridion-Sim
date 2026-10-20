#include "DebugOverlay.h"

#include "Assets.h"
#include "Theme.h"
#include "core/SpatialHash.h"

#include <raylib.h>

#include <cmath>
#include <cstdio>

namespace gridiron {
namespace {

// JetBrains Mono at raylib size 22: every glyph advance is exactly 10 px (0.6 em advance over a
// 1.32 em ascender-descender), so fixed-width printf fields line up as true columns.
constexpr int kLineHeight = 24;
constexpr int kPanelLeft = 8;
constexpr int kPanelTop = 8;
constexpr int kPadding = 12;

// One line of text on whole pixels. A fixed char buffer avoids heap allocations every frame.
void drawLine(const Font& font, int line, const char* text) {
    const Vector2 position{static_cast<float>(kPanelLeft + kPadding),
                           static_cast<float>(kPanelTop + kPadding / 2 + line * kLineHeight)};
    DrawTextEx(font, text, position, static_cast<float>(Assets::kDebugFontSize), 0.0f,
               theme::kLineWhite);
}

} // namespace

void DebugOverlay::record(const FrameSample& sample) {
    m_window.seconds += sample.frameSeconds;
    m_window.frames += 1;
    m_window.updateSeconds += sample.updateSeconds;
    m_window.steps += sample.steps;
    m_window.collisionChecks += sample.collisionChecks;

    if (m_window.seconds < kRefreshSeconds) {
        return;
    }
    const float frames = static_cast<float>(m_window.frames);
    m_shown.fps = frames / m_window.seconds;
    m_shown.stepsPerFrame = static_cast<float>(m_window.steps) / frames;
    m_shown.collisionChecks = static_cast<float>(m_window.collisionChecks) / frames;
    if (m_window.steps > 0) { // keep the last value through a window with no steps
        m_shown.updateMicroseconds =
            static_cast<float>(m_window.updateSeconds * 1.0e6 / m_window.steps);
    }
    m_window = Window{};
}

void DebugOverlay::drawGrid(const SpatialHash& grid) const {
    if (!m_enabled) {
        return;
    }
    const float cell = grid.cellSize();
    const Vec2 origin = grid.origin();
    const float width = cell * static_cast<float>(grid.columns());
    const float height = cell * static_cast<float>(grid.rows());

    // Tint occupied cells: one player = faint, a crowd = strong.
    for (int r = 0; r < grid.rows(); ++r) {
        for (int c = 0; c < grid.columns(); ++c) {
            const int count = grid.countInCell(c, r);
            if (count > 0) {
                const float alpha = count == 1 ? 0.15f : 0.35f;
                DrawRectangleRec(Rectangle{origin.x + cell * static_cast<float>(c),
                                           origin.y + cell * static_cast<float>(r), cell, cell},
                                 Fade(theme::kLineWhite, alpha));
            }
        }
    }

    const Color lineColor = theme::shadow(0.3f);
    for (int c = 0; c <= grid.columns(); ++c) {
        const float x = origin.x + cell * static_cast<float>(c);
        DrawLineEx(Vector2{x, origin.y}, Vector2{x, origin.y + height}, 0.05f, lineColor);
    }
    for (int r = 0; r <= grid.rows(); ++r) {
        const float y = origin.y + cell * static_cast<float>(r);
        DrawLineEx(Vector2{origin.x, y}, Vector2{origin.x + width, y}, 0.05f, lineColor);
    }
}

void DebugOverlay::drawStats(const Assets& assets, const OverlaySettings& settings) const {
    if (!m_enabled) {
        return;
    }
    constexpr int kLines = 6;
    DrawRectangle(kPanelLeft, kPanelTop, kPanelRight - kPanelLeft, kLines * kLineHeight + kPadding,
                  theme::panel(0.75f));

    const Font& font = assets.debugFont();
    char line[64];
    char detail[16];

    // Every line has the same four fixed-width columns: label (14), value (7, right-aligned),
    // detail (10), key. A value changing width can never move anything else.
    constexpr const char* kFormat = "%-14s%7s  %-10s%s";
    char value[16];

    std::snprintf(value, sizeof(value), "%.1f", static_cast<double>(m_shown.fps));
    if (settings.fpsCap > 0) {
        std::snprintf(detail, sizeof(detail), "cap %4d", settings.fpsCap);
    } else {
        std::snprintf(detail, sizeof(detail), "cap  off");
    }
    std::snprintf(line, sizeof(line), kFormat, "FPS", value, detail, "F3");
    drawLine(font, 0, line);

    std::snprintf(value, sizeof(value), "%.1f", static_cast<double>(m_shown.updateMicroseconds));
    std::snprintf(line, sizeof(line), kFormat, "Update", value, "us/step", "");
    drawLine(font, 1, line);

    std::snprintf(value, sizeof(value), "%.2f", static_cast<double>(m_shown.stepsPerFrame));
    std::snprintf(line, sizeof(line), kFormat, "Steps/frame", value, "@ 60 Hz", "");
    drawLine(font, 2, line);

    const int bruteForce = settings.players * (settings.players - 1) / 2;
    std::snprintf(value, sizeof(value), "%.0f", static_cast<double>(m_shown.collisionChecks));
    std::snprintf(detail, sizeof(detail), "of %4d", bruteForce);
    std::snprintf(line, sizeof(line), kFormat, "Coll. checks", value, detail, "");
    drawLine(font, 3, line);

    std::snprintf(line, sizeof(line), kFormat, "Polish", settings.polish ? "on" : "off", "", "F2");
    drawLine(font, 4, line);

    std::snprintf(line, sizeof(line), kFormat, "Debug", "on", "", "F1");
    drawLine(font, 5, line);
}

} // namespace gridiron
