#include "DebugOverlay.h"

#include "core/SpatialHash.h"

#include <raylib.h>

#include <cstdio>

namespace gridiron {
namespace {

constexpr int kFontSize = 18;
constexpr int kLineHeight = 22;
constexpr int kPadding = 8;

// One line of text. A fixed char buffer avoids heap allocations every frame.
void drawLine(int line, const char* text) {
    DrawText(text, kPadding * 2, kPadding * 2 + line * kLineHeight, kFontSize, RAYWHITE);
}

} // namespace

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
                                 Fade(ORANGE, alpha));
            }
        }
    }

    const Color lineColor = Fade(BLACK, 0.25f);
    for (int c = 0; c <= grid.columns(); ++c) {
        const float x = origin.x + cell * static_cast<float>(c);
        DrawLineEx(Vector2{x, origin.y}, Vector2{x, origin.y + height}, 0.05f, lineColor);
    }
    for (int r = 0; r <= grid.rows(); ++r) {
        const float y = origin.y + cell * static_cast<float>(r);
        DrawLineEx(Vector2{origin.x, y}, Vector2{origin.x + width, y}, 0.05f, lineColor);
    }
}

void DebugOverlay::drawStats(const FrameStats& stats) const {
    const int lines = m_enabled ? 4 : 3;
    DrawRectangle(kPadding, kPadding, 340, lines * kLineHeight + kPadding * 2, Fade(BLACK, 0.55f));

    char buffer[96];
    if (stats.fpsCap > 0) {
        std::snprintf(buffer, sizeof(buffer), "FPS: %d (cap %d, F3)", stats.fps, stats.fpsCap);
    } else {
        std::snprintf(buffer, sizeof(buffer), "FPS: %d (uncapped, F3)", stats.fps);
    }
    drawLine(0, buffer);

    std::snprintf(buffer, sizeof(buffer), "Update: %.3f ms @ 60 Hz", stats.updateMs);
    drawLine(1, buffer);

    std::snprintf(buffer, sizeof(buffer), "Steps this frame: %d   Debug: %s (F1)",
                  stats.stepsThisFrame, m_enabled ? "on" : "off");
    drawLine(2, buffer);

    if (m_enabled) {
        std::snprintf(buffer, sizeof(buffer), "Collision checks: %d (brute force: %d)",
                      stats.collisionChecks, stats.bruteForcePairs);
        drawLine(3, buffer);
    }
}

} // namespace gridiron
