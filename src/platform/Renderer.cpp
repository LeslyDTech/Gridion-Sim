#include "Renderer.h"

#include "FollowCamera.h"
#include "core/FieldGeometry.h"
#include "core/Game.h"
#include "core/World.h"

#include <raylib.h>

#include <cstdio>

namespace gridiron::render {
namespace {

// --- Palette ---
constexpr Color kTurf{52, 128, 60, 255};
constexpr Color kTurfStripe{47, 118, 55, 255};
constexpr Color kSidelineArea{34, 82, 42, 255};
constexpr Color kOwnEndZone{32, 64, 128, 255};
constexpr Color kOpponentEndZone{136, 34, 34, 255};
constexpr Color kChalk{240, 240, 240, 255};
constexpr Color kLineOfScrimmage{80, 150, 255, 255};
constexpr Color kOffenseJersey{30, 64, 160, 255};
constexpr Color kDefenseJersey{235, 235, 235, 255};
constexpr Color kDefenseTrim{170, 30, 30, 255};
constexpr Color kCarrierRing{255, 215, 0, 255};
constexpr Color kBallBrown{140, 80, 35, 255};

constexpr float kLineWidth = 0.12f;     // painted lines are 4 inches wide
constexpr float kBoundaryWidth = 0.35f; // sidelines and end lines
constexpr float kHashLength = 0.67f;    // 2 feet

Vector2 toRaylib(Vec2 v) { return Vector2{v.x, v.y}; }

Vec2 interpolated(const Player& player, float alpha) {
    return lerp(player.previousPosition, player.position, alpha);
}

void drawVerticalLine(float x, float y0, float y1, float width, Color color = kChalk) {
    DrawLineEx(Vector2{x, y0}, Vector2{x, y1}, width, color);
}

// Text centered on a screen point.
void drawCenteredText(const char* text, Vector2 center, int fontSize, Color color) {
    const int width = MeasureText(text, fontSize);
    DrawText(text, static_cast<int>(center.x) - width / 2,
             static_cast<int>(center.y) - fontSize / 2, fontSize, color);
}

// ---------------------------------------------------------------- field

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

// ---------------------------------------------------------------- HUD helpers

const char* resultText(PlayResult result) {
    switch (result) {
    case PlayResult::Tackled:
        return "TACKLED";
    case PlayResult::OutOfBounds:
        return "OUT OF BOUNDS";
    case PlayResult::Touchdown:
        return "TOUCHDOWN!";
    case PlayResult::Safety:
        return "SAFETY";
    case PlayResult::Incomplete:
        return "INCOMPLETE";
    case PlayResult::None:
        break;
    }
    return "";
}

// "Own 25" / "Opp 40" / "50" for the HUD.
void formatBallOn(float x, char* buffer, std::size_t size) {
    const int line = field::yardLineNumber(x);
    if (line == 50) {
        std::snprintf(buffer, size, "50");
    } else {
        std::snprintf(buffer, size, "%s %d", x < field::kLength / 2.0f ? "Own" : "Opp", line);
    }
}

void drawResultBanner(const PlayOutcome& outcome) {
    const int screenWidth = GetScreenWidth();
    const int y = GetScreenHeight() / 3;
    DrawRectangle(0, y - 40, screenWidth, 80, Fade(BLACK, 0.6f));

    char buffer[64];
    if (outcome.result == PlayResult::Incomplete) {
        std::snprintf(buffer, sizeof(buffer), "INCOMPLETE - no gain");
    } else {
        std::snprintf(buffer, sizeof(buffer), "%s  %+.1f yards", resultText(outcome.result),
                      static_cast<double>(outcome.yardsGained));
    }
    drawCenteredText(buffer, Vector2{static_cast<float>(screenWidth) / 2.0f, static_cast<float>(y)},
                     40, RAYWHITE);
}

} // namespace

// ================================================================ world space

void drawFieldSurface() {
    drawTurf();
    drawYardLines();
    drawHashMarks();
    DrawRectangleLinesEx(Rectangle{0.0f, 0.0f, field::kLength, field::kWidth}, kBoundaryWidth,
                         kChalk);
}

void drawLineOfScrimmage(const World& world) {
    drawVerticalLine(world.lineOfScrimmageX(), 0.0f, field::kWidth, 0.2f,
                     Fade(kLineOfScrimmage, 0.8f));
}

void drawRoutes(const World& world) {
    if (world.phase() != PlayPhase::PreSnap) {
        return;
    }
    for (const Player& player : world.players()) {
        Vec2 from = player.position;
        for (const Vec2 waypoint : player.route) {
            DrawLineEx(toRaylib(from), toRaylib(waypoint), 0.18f, Fade(YELLOW, 0.55f));
            from = waypoint;
        }
        if (!player.route.empty()) {
            DrawCircleV(toRaylib(player.route.back()), 0.3f, Fade(YELLOW, 0.75f));
        }
    }
}

void drawPlayers(const World& world, float alpha) {
    for (const Player& player : world.players()) {
        const Vector2 center = toRaylib(interpolated(player, alpha));
        const float radius = player.attributes.radius;
        const bool offense = player.team == Team::Offense;

        if (player.state == PlayerState::BallCarrier) {
            DrawCircleV(center, radius + 0.3f, kCarrierRing);
        }
        DrawCircleV(center, radius, offense ? kOffenseJersey : kDefenseJersey);
        DrawCircleLinesV(center, radius, offense ? kChalk : kDefenseTrim);
        if (player.state == PlayerState::Tackled) {
            DrawCircleV(center, radius * 0.4f, RED);
        }
    }
}

void drawBall(const World& world, float alpha) {
    const Ball& ball = world.ball();
    const Vec2 ground = lerp(ball.previousPosition, ball.position, alpha);
    if (ball.state == BallState::InAir) {
        // Shadow stays on the ground; the ball itself is lifted by its flight height.
        DrawCircleV(toRaylib(ground), 0.25f, Fade(BLACK, 0.35f));
        const Vec2 lifted = ground - Vec2{0.0f, ball.height()};
        DrawCircleV(toRaylib(lifted), 0.3f, kBallBrown);
        DrawCircleLinesV(toRaylib(lifted), 0.3f, kChalk);
    } else if (ball.state == BallState::Spotted || ball.state == BallState::Dead) {
        DrawCircleV(toRaylib(ground), 0.3f, kBallBrown);
    }
    // While Held, the yellow carrier ring shows who has it.
}

// ================================================================ screen space

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

void drawPlayerLabels(const World& world, float alpha, const FollowCamera& camera,
                      bool showStates) {
    const float pixelsPerYard = camera.pixelsPerYard();
    const bool canThrow =
        world.ball().state == BallState::Held || world.phase() == PlayPhase::PreSnap;
    char buffer[16];

    for (const Player& player : world.players()) {
        const Vector2 center = camera.worldToScreen(interpolated(player, alpha));
        const bool offense = player.team == Team::Offense;

        // Jersey number, sized to fit inside the circle.
        const int fontSize = static_cast<int>(player.attributes.radius * pixelsPerYard * 1.1f);
        std::snprintf(buffer, sizeof(buffer), "%d", player.jersey);
        drawCenteredText(buffer, center, fontSize, offense ? RAYWHITE : kDefenseTrim);

        // Throw-button badge above each eligible receiver.
        if (player.receiverNumber != Player::kNone && canThrow &&
            (player.state == PlayerState::PreSnap || player.state == PlayerState::RunningRoute ||
             player.state == PlayerState::Idle)) {
            const Vector2 badge{center.x,
                                center.y - (player.attributes.radius + 0.7f) * pixelsPerYard};
            DrawCircleV(badge, 9.0f, Fade(BLACK, 0.7f));
            std::snprintf(buffer, sizeof(buffer), "%d", player.receiverNumber + 1);
            drawCenteredText(buffer, badge, 14, YELLOW);
        }

        if (showStates) {
            const Vector2 below{center.x,
                                center.y + (player.attributes.radius + 0.5f) * pixelsPerYard};
            drawCenteredText(toString(player.state), below, 12, Fade(RAYWHITE, 0.9f));
        }
    }
}

void drawHud(const Game& game) {
    const World& world = game.world();
    const int screenWidth = GetScreenWidth();
    const int screenHeight = GetScreenHeight();

    // Top center: play call and ball position.
    char ballOn[16];
    formatBallOn(world.lineOfScrimmageX(), ballOn, sizeof(ballOn));
    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "%s  vs  %s   |   Ball on %s",
                  game.currentPlay().name.c_str(), game.defense().name.c_str(), ballOn);
    const int width = MeasureText(buffer, 20);
    DrawRectangle(screenWidth / 2 - width / 2 - 12, 8, width + 24, 32, Fade(BLACK, 0.55f));
    DrawText(buffer, screenWidth / 2 - width / 2, 14, 20, RAYWHITE);

    // Bottom: context-sensitive controls.
    const char* help = "";
    switch (world.phase()) {
    case PlayPhase::PreSnap:
        help = "SPACE / (A): snap     TAB / D-pad right: next play     F1: debug view";
        break;
    case PlayPhase::Live:
        help = world.ball().state == BallState::Held &&
                       world.players()[static_cast<std::size_t>(world.ball().carrierId)].role ==
                           Role::QB
                   ? "1-5 / (X)(Y)(B)(RB)(LB): throw     Arrows / WASD / left stick: scramble"
                   : "Arrows / WASD / left stick: run";
        break;
    case PlayPhase::Over:
        help = "SPACE: next play";
        break;
    }
    DrawRectangle(0, screenHeight - 32, screenWidth, 32, Fade(BLACK, 0.55f));
    drawCenteredText(
        help,
        Vector2{static_cast<float>(screenWidth) / 2.0f, static_cast<float>(screenHeight) - 16.0f},
        18, RAYWHITE);

    if (world.phase() == PlayPhase::Over) {
        drawResultBanner(world.outcome());
    }
}

} // namespace gridiron::render
