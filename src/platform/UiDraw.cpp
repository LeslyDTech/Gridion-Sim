#include "UiDraw.h"

#include "Assets.h"

#include <algorithm>
#include <cmath>
#include <cstring>

namespace gridiron::ui {

using theme::hud::TextStyle;

float pixelSize(const TextStyle& style) { return style.size * kFigmaToRaylibFontScale; }

const Font& fontFor(const Assets& assets, const TextStyle& style) {
    return assets.hudFont(style.weight, pixelSize(style));
}

Vector2 measure(const Assets& assets, const TextStyle& style, const char* text) {
    return MeasureTextEx(fontFor(assets, style), text, pixelSize(style), 0.0f);
}

float glyphTop(float boxTop, const TextStyle& style, float lineHeight) {
    // CSS centers the font's content area (1.2 em for this font) inside each line box.
    return boxTop + (lineHeight - kFigmaToRaylibFontScale) * style.size / 2.0f;
}

void drawText(const Assets& assets, const TextStyle& style, const char* text, Vector2 topLeft,
              Color color) {
    DrawTextEx(fontFor(assets, style), text, Vector2{std::round(topLeft.x), std::round(topLeft.y)},
               pixelSize(style), 0.0f, color);
}

void drawTextInBox(const Assets& assets, const TextStyle& style, const char* text, Rectangle box,
                   float paddingX, float align) {
    const Vector2 size = measure(assets, style, text);
    const float x = box.x + paddingX + (box.width - 2.0f * paddingX - size.x) * align;
    const float y = box.y + (box.height - size.y) / 2.0f;
    drawText(assets, style, text, Vector2{x, y}, style.color);
}

float drawParagraph(const Assets& assets, const TextStyle& style, const char* text, Vector2 topLeft,
                    float width, float lineHeight, Color color) {
    const float lineStep = lineHeight * style.size;
    char line[256];
    std::size_t lineLength = 0;
    float y = topLeft.y;
    int lines = 0;

    auto flush = [&] {
        line[lineLength] = '\0';
        drawText(assets, style, line, Vector2{topLeft.x, glyphTop(y, style, lineHeight)}, color);
        y += lineStep;
        ++lines;
        lineLength = 0;
    };

    const char* word = text;
    while (*word != '\0') {
        while (*word == ' ') {
            ++word;
        }
        const char* end = word;
        while (*end != '\0' && *end != ' ') {
            ++end;
        }
        const auto wordLength = static_cast<std::size_t>(end - word);
        if (wordLength == 0) {
            break;
        }
        // Try the line with this word appended; wrap first if it would overflow.
        char candidate[256];
        std::size_t candidateLength = lineLength;
        std::memcpy(candidate, line, lineLength);
        if (candidateLength > 0) {
            candidate[candidateLength++] = ' ';
        }
        const std::size_t copy = std::min(wordLength, sizeof(candidate) - 1 - candidateLength);
        std::memcpy(candidate + candidateLength, word, copy);
        candidateLength += copy;
        candidate[candidateLength] = '\0';

        if (lineLength > 0 && measure(assets, style, candidate).x > width) {
            flush();
            std::memcpy(line, word, copy);
            lineLength = copy;
        } else {
            std::memcpy(line, candidate, candidateLength);
            lineLength = candidateLength;
        }
        word = end;
    }
    if (lineLength > 0) {
        flush();
    }
    return static_cast<float>(lines) * lineStep;
}

void fillRounded(Rectangle rect, float radius, Color color) {
    const float shortSide = std::min(rect.width, rect.height);
    DrawRectangleRounded(rect, shortSide > 0.0f ? 2.0f * radius / shortSide : 0.0f, 8, color);
}

void fillClippedToRounded(Rectangle parent, float radius, Rectangle child, Color color) {
    BeginScissorMode(static_cast<int>(std::floor(child.x)), static_cast<int>(std::floor(child.y)),
                     static_cast<int>(std::ceil(child.width)),
                     static_cast<int>(std::ceil(child.height)));
    fillRounded(parent, radius, color);
    EndScissorMode();
}

float screenWidth() { return static_cast<float>(GetScreenWidth()); }
float screenHeight() { return static_cast<float>(GetScreenHeight()); }

void toUpper(const char* text, char* out, std::size_t size) {
    std::size_t i = 0;
    for (; text[i] != '\0' && i + 1 < size; ++i) {
        const char c = text[i];
        out[i] = (c >= 'a' && c <= 'z') ? static_cast<char>(c - 'a' + 'A') : c;
    }
    out[i] = '\0';
}

} // namespace gridiron::ui
