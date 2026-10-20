#include "Renderer.h"

#include "FollowCamera.h"
#include "core/FieldGeometry.h"

#include <raylib.h>

#include <cstdio>

namespace gridiron::render {
namespace {

constexpr Color kTurf{52, 128, 60, 255};
constexpr Color kTurfStripe{47, 118, 55, 255};
constexpr Color kSidelineArea{34, 82, 42, 255};
constexpr Color kOwnEndZone{32, 64, 128, 255};
constexpr Color kOpponentEndZone{136, 34, 34, 255};
constexpr Color kChalk{240, 240, 240, 255};

constexpr float kLineWidth = 0.12f;     // painted lines are 4 inches wide
constexpr float kBoundaryWidth = 0.35f; // sidelines and end lines
constexpr float kHashLength = 0.67f;    // 2 feet

void drawVerticalLine(float x, float y0, float y1, float width) {
    DrawLineEx(Vector2{x, y0}, Vector2{x, y1}, width, kChalk);
}

void drawTurf() {
    using namespace field;
    // Sideline area well beyond the field so the camera margin never shows the clear color.
    DrawRectangleRec(Rectangle{-20.0f, -20.0f, kLength + 40.0f, kWidth + 40.0f}, kSidelineArea);
    DrawRectangleRec(Rectangle{kOwnGoalLine, 0.0f, 100.0f, kWidth}, kTurf);

    // Alternate mowing stripes every 5 yards, like a real stadium.
    for (int i = 0; i < 20; i += 2) {
        const float x = kOwnGoalLine + static_cast<float>(i) * 5.0f;
        DrawRectangleRec(Rectangle{x, 0.0f, 5.0f, kWidth}, kTurfStripe);
    }

    DrawRectangleRec(Rectangle{0.0f, 0.0f, kEndZoneDepth, kWidth}, kOwnEndZone);
    DrawRectangleRec(Rectangle{kOpponentGoalLine, 0.0f, kEndZoneDepth, kWidth}, kOpponentEndZone);
}

void drawBoundary() {
    using namespace field;
    DrawRectangleLinesEx(Rectangle{0.0f, 0.0f, kLength, kWidth}, kBoundaryWidth, kChalk);
}

void drawYardLines() {
    using namespace field;
    for (int yards = 0; yards <= 100; yards += 5) {
        const bool isGoalLine = yards == 0 || yards == 100;
        const float x = kOwnGoalLine + static_cast<float>(yards);
        drawVerticalLine(x, 0.0f, kWidth, isGoalLine ? kBoundaryWidth : kLineWidth);
    }
}

void drawHashMarks() {
    using namespace field;
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
}

// Draws one character centered on a world point, optionally upside down.
void drawGlyph(const FollowCamera& camera, char glyph, Vec2 world, float rotationDegrees) {
    const char text[2] = {glyph, '\0'};
    const float fontSize = 2.0f * camera.pixelsPerYard(); // numbers are 2 yards tall
    const float spacing = 0.0f;
    const Vector2 size = MeasureTextEx(GetFontDefault(), text, fontSize, spacing);
    const Vector2 origin{size.x / 2.0f, size.y / 2.0f};
    DrawTextPro(GetFontDefault(), text, camera.worldToScreen(world), origin, rotationDegrees,
                fontSize, spacing, Fade(kChalk, 0.85f));
}

} // namespace

void drawFieldSurface() {
    drawTurf();
    drawYardLines();
    drawHashMarks();
    drawBoundary();
}

void drawFieldNumbers(const FollowCamera& camera) {
    using namespace field;
    // Real numbers straddle the line: tens digit on one side, ones digit on the other.
    constexpr float kDigitOffset = 0.9f;
    constexpr float kFromSideline = 11.0f; // number centers, ~12 yards to the top of the digits
    for (int yards = 10; yards <= 90; yards += 10) {
        const float x = kOwnGoalLine + static_cast<float>(yards);
        char digits[3];
        std::snprintf(digits, sizeof(digits), "%d", field::yardLineNumber(x));
        const char tens = digits[0];
        const char ones = digits[1];

        // Bottom of the screen: upright, read left to right.
        const float bottomY = kWidth - kFromSideline;
        drawGlyph(camera, tens, {x - kDigitOffset, bottomY}, 0.0f);
        drawGlyph(camera, ones, {x + kDigitOffset, bottomY}, 0.0f);

        // Top of the screen: upside down, as seen from the far sideline.
        drawGlyph(camera, tens, {x + kDigitOffset, kFromSideline}, 180.0f);
        drawGlyph(camera, ones, {x - kDigitOffset, kFromSideline}, 180.0f);
    }
}

} // namespace gridiron::render
