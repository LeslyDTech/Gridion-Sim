#include "FieldRenderer.h"

#include "Assets.h"
#include "Theme.h"
#include "core/FieldGeometry.h"

#include <raylib.h>

#include <algorithm>
#include <cstdio>

namespace gridiron::render {
namespace {

constexpr float kLineWidth = 0.12f;     // painted lines are 4 inches wide
constexpr float kBoundaryWidth = 0.35f; // sidelines and end lines
constexpr float kHashLength = 0.67f;    // 2 feet
constexpr float kNumberHeight = 2.0f;   // yard numbers are 6 feet tall
constexpr float kNumberFromSideline = 11.0f;

void drawVerticalLine(float x, float y0, float y1, float width) {
    DrawLineEx(Vector2{x, y0}, Vector2{x, y1}, width, theme::kLineWhite);
}

// Text centered on a world point, `height` yards tall, rotated about its center.
void drawWorldText(const Font& font, const char* text, Vector2 center, float height,
                   float rotationDegrees, Color color) {
    const float spacing = height * 0.05f;
    const Vector2 size = MeasureTextEx(font, text, height, spacing);
    DrawTextPro(font, text, center, Vector2{size.x / 2.0f, size.y / 2.0f}, rotationDegrees, height,
                spacing, color);
}

// Covers the 100-yard field with the grass tile: one 64 px tile = 5 x 5 yards, the scale the
// asset was designed for. The last row is cropped because 53.3 yards isn't a multiple of 5.
void drawGrassTiles(const Texture2D& tiles) {
    using namespace field;
    constexpr float kTileYards = 5.0f;
    constexpr float kTilePixels = 64.0f;
    for (float y = 0.0f; y < kWidth; y += kTileYards) {
        const float rowYards = std::min(kTileYards, kWidth - y);
        const Rectangle source{0.0f, 0.0f, kTilePixels, kTilePixels * rowYards / kTileYards};
        for (float x = kOwnGoalLine; x < kOpponentGoalLine; x += kTileYards) {
            DrawTexturePro(tiles, source, Rectangle{x, y, kTileYards, rowYards}, Vector2{}, 0.0f,
                           WHITE); // WHITE = draw the texture's own colors, untinted
        }
    }
}

void drawTurf(const RenderContext& context) {
    using namespace field;
    // Sideline area well beyond the field so the camera margin never shows the clear color.
    DrawRectangleRec(Rectangle{-20.0f, -20.0f, kLength + 40.0f, kWidth + 40.0f},
                     theme::sidelineArea());

    const Texture2D* grass = context.assets.grassTiles();
    if (context.polish && grass != nullptr) {
        drawGrassTiles(*grass);
    } else {
        DrawRectangleRec(Rectangle{kOwnGoalLine, 0.0f, 100.0f, kWidth}, theme::kGrassLight);
    }

    if (context.polish) {
        // Mowing pattern: every other 5-yard band slightly darker, like a broadcast stadium.
        for (int i = 0; i < 20; i += 2) {
            const float x = kOwnGoalLine + static_cast<float>(i) * 5.0f;
            DrawRectangleRec(Rectangle{x, 0.0f, 5.0f, kWidth}, theme::shadow(0.07f));
        }
    }

    DrawRectangleRec(Rectangle{0.0f, 0.0f, kEndZoneDepth, kWidth}, theme::kHome);
    DrawRectangleRec(Rectangle{kOpponentGoalLine, 0.0f, kEndZoneDepth, kWidth}, theme::kAway);
}

void drawEndZoneNames(const Font& font) {
    using namespace field;
    const float height = kEndZoneDepth * 0.6f;
    const Color paint = Fade(theme::kLineWhite, 0.9f);
    // Each name reads from its own goal line outward, like a painted end zone.
    drawWorldText(font, theme::kHomeName, Vector2{kEndZoneDepth / 2.0f, kCenterY}, height, -90.0f,
                  paint);
    drawWorldText(font, theme::kAwayName, Vector2{kLength - kEndZoneDepth / 2.0f, kCenterY}, height,
                  90.0f, paint);
}

void drawLines() {
    using namespace field;
    for (int yards = 0; yards <= 100; yards += 5) {
        const bool isGoalLine = yards == 0 || yards == 100;
        const float x = kOwnGoalLine + static_cast<float>(yards);
        drawVerticalLine(x, 0.0f, kWidth, isGoalLine ? kBoundaryWidth : kLineWidth);
    }

    const float half = kHashLength / 2.0f;
    for (int yards = 1; yards < 100; ++yards) {
        if (yards % 5 == 0) {
            continue; // full yard line already drawn
        }
        const float x = kOwnGoalLine + static_cast<float>(yards);
        drawVerticalLine(x, kNearHashY - half, kNearHashY + half, kLineWidth);
        drawVerticalLine(x, kFarHashY - half, kFarHashY + half, kLineWidth);
        // Short "inbounds" ticks along both sidelines.
        drawVerticalLine(x, 0.33f, 0.33f + kHashLength, kLineWidth);
        drawVerticalLine(x, kWidth - 0.33f - kHashLength, kWidth - 0.33f, kLineWidth);
    }

    DrawRectangleLinesEx(Rectangle{0.0f, 0.0f, kLength, kWidth}, kBoundaryWidth, theme::kLineWhite);
}

// The small painted triangle next to each number points toward the nearer goal line.
void drawDirectionArrow(float x, float y, bool towardOwnGoal, bool upsideDown) {
    const float tip = towardOwnGoal ? -1.0f : 1.0f;
    const float ax = x + tip * 2.6f;
    const float dy = upsideDown ? 0.6f : -0.6f; // sits near the top of the digits
    const Vector2 point{ax + tip * 0.6f, y + dy};
    const Vector2 a{ax, y + dy - 0.35f};
    const Vector2 b{ax, y + dy + 0.35f};
    // raylib triangles must be counter-clockwise on screen.
    if (tip > 0.0f) {
        DrawTriangle(a, b, point, Fade(theme::kLineWhite, 0.85f));
    } else {
        DrawTriangle(b, a, point, Fade(theme::kLineWhite, 0.85f));
    }
}

void drawYardNumbers(const Font& font, bool polish) {
    using namespace field;
    // Real numbers straddle the line: tens digit on one side, ones digit on the other.
    constexpr float kDigitOffset = 0.9f;
    const Color paint = Fade(theme::kLineWhite, 0.85f);
    for (int yards = 10; yards <= 90; yards += 10) {
        const float x = kOwnGoalLine + static_cast<float>(yards);
        const int number = yardLineNumber(x);
        const char tens[2] = {static_cast<char>('0' + number / 10), '\0'};
        const char ones[2] = {static_cast<char>('0' + number % 10), '\0'};

        // Near side: upright. Far side: upside down, as seen from the other sideline.
        const float nearY = kWidth - kNumberFromSideline;
        const float farY = kNumberFromSideline;
        drawWorldText(font, tens, {x - kDigitOffset, nearY}, kNumberHeight, 0.0f, paint);
        drawWorldText(font, ones, {x + kDigitOffset, nearY}, kNumberHeight, 0.0f, paint);
        drawWorldText(font, tens, {x + kDigitOffset, farY}, kNumberHeight, 180.0f, paint);
        drawWorldText(font, ones, {x - kDigitOffset, farY}, kNumberHeight, 180.0f, paint);

        if (polish && number != 50) {
            const bool towardOwnGoal = yards < 50;
            drawDirectionArrow(x, nearY, towardOwnGoal, false);
            drawDirectionArrow(x, farY, towardOwnGoal, true);
        }
    }
}

} // namespace

void drawField(const RenderContext& context) {
    // Polish off uses raylib's built-in pixel font: the plain Milestone 1 look.
    const Font& font = context.polish ? context.assets.font() : GetFontDefault();
    drawTurf(context);
    if (context.polish) {
        drawEndZoneNames(font);
    }
    drawLines();
    drawYardNumbers(font, context.polish);
}

void drawVignette(const RenderContext& context) {
    if (!context.polish) {
        return;
    }
    const Texture2D& texture = context.assets.vignette();
    const Rectangle source{0.0f, 0.0f, static_cast<float>(texture.width),
                           static_cast<float>(texture.height)};
    const Rectangle dest{0.0f, 0.0f, static_cast<float>(GetScreenWidth()),
                         static_cast<float>(GetScreenHeight())};
    DrawTexturePro(texture, source, dest, Vector2{}, 0.0f, Fade(theme::kLineWhite, 0.55f));
}

} // namespace gridiron::render
