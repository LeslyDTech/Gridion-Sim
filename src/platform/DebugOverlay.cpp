#include "DebugOverlay.h"

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

void DebugOverlay::drawStats(const FrameStats& stats) const {
    const int lines = 3;
    DrawRectangle(kPadding, kPadding, 300, lines * kLineHeight + kPadding * 2, Fade(BLACK, 0.55f));

    char buffer[96];
    if (stats.fpsCap > 0) {
        std::snprintf(buffer, sizeof(buffer), "FPS: %d (cap %d, F2)", stats.fps, stats.fpsCap);
    } else {
        std::snprintf(buffer, sizeof(buffer), "FPS: %d (uncapped, F2)", stats.fps);
    }
    drawLine(0, buffer);

    std::snprintf(buffer, sizeof(buffer), "Update: %.3f ms @ 60 Hz", stats.updateMs);
    drawLine(1, buffer);

    std::snprintf(buffer, sizeof(buffer), "Steps this frame: %d   Debug: %s (F1)",
                  stats.stepsThisFrame, m_enabled ? "on" : "off");
    drawLine(2, buffer);
}

} // namespace gridiron
