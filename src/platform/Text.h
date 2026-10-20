#pragma once

#include <raylib.h>

namespace gridiron::text {

// Draws `text` in `font`, `size` pixels tall, centered on a screen point.
inline void drawCentered(const Font& font, const char* text, Vector2 center, float size,
                         Color color) {
    const float spacing = size * 0.05f;
    const Vector2 measured = MeasureTextEx(font, text, size, spacing);
    DrawTextEx(font, text, Vector2{center.x - measured.x / 2.0f, center.y - measured.y / 2.0f},
               size, spacing, color);
}

} // namespace gridiron::text
