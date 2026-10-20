#include "Hud.h"

#include "Assets.h"
#include "DebugOverlay.h"
#include "Theme.h"
#include "core/FieldGeometry.h"
#include "core/Game.h"

#include <raylib.h>

#include <cstdio>

namespace gridiron::render {
namespace {

constexpr int kHelpBarHeight = 32;
constexpr float kScorebugHeight = 48.0f;
constexpr int kScorebugMargin = 14;

// ---------------------------------------------------------------- formatting (no allocations)

const char* ordinal(int down) {
    switch (down) {
    case 1:
        return "1ST";
    case 2:
        return "2ND";
    case 3:
        return "3RD";
    default:
        return "4TH";
    }
}

// "1ST & 10" / "3RD & GOAL"
void formatDownAndDistance(const Game& game, char* buffer, std::size_t size) {
    const Drive& drive = game.drive();
    if (drive.goalToGo()) {
        std::snprintf(buffer, size, "%s & GOAL", ordinal(drive.down()));
    } else {
        std::snprintf(buffer, size, "%s & %d", ordinal(drive.down()),
                      drive.yardsToGo(game.world().lineOfScrimmageX()));
    }
}

// "OWN 25" / "OPP 40" / "50"
void formatBallOn(float x, char* buffer, std::size_t size) {
    const int line = field::yardLineNumber(x);
    if (line == 50) {
        std::snprintf(buffer, size, "50");
    } else {
        std::snprintf(buffer, size, "%s %d", x < field::kLength / 2.0f ? "OWN" : "OPP", line);
    }
}

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

const char* controlsHelp(const World& world) {
    switch (world.phase()) {
    case PlayPhase::PreSnap:
        return "SPACE / (A): snap     TAB / D-pad right: next play     F1: debug   F2: polish";
    case PlayPhase::Live: {
        const Ball& ball = world.ball();
        const bool qbHasBall =
            ball.state == BallState::Held &&
            world.players()[static_cast<std::size_t>(ball.carrierId)].role == Role::QB;
        return qbHasBall ? "1-5 / (X)(Y)(B)(RB)(LB): throw     Arrows / WASD / left stick: scramble"
                         : "Arrows / WASD / left stick: run";
    }
    case PlayPhase::Over:
        return "SPACE: next play";
    }
    return "";
}

// ---------------------------------------------------------------- shared pieces

// Horizontal center of the top area to the right of the debug stats panel.
float topBarCenterX() {
    return (static_cast<float>(DebugOverlay::kPanelRight) + static_cast<float>(GetScreenWidth())) /
           2.0f;
}

void drawCenteredText(const char* text, Vector2 center, int fontSize, Color color) {
    const int width = MeasureText(text, fontSize);
    DrawText(text, static_cast<int>(center.x) - width / 2,
             static_cast<int>(center.y) - fontSize / 2, fontSize, color);
}

void drawControlsHelp(const World& world) {
    const int screenWidth = GetScreenWidth();
    const int screenHeight = GetScreenHeight();
    DrawRectangle(0, screenHeight - kHelpBarHeight, screenWidth, kHelpBarHeight,
                  theme::panel(0.6f));
    drawCenteredText(controlsHelp(world),
                     Vector2{static_cast<float>(screenWidth) / 2.0f,
                             static_cast<float>(screenHeight - kHelpBarHeight / 2)},
                     18, theme::kLineWhite);
}

// ---------------------------------------------------------------- plain HUD (polish off)

void drawPlainTopBar(const Game& game, int playClockSeconds) {
    char downAndDistance[24];
    char ballOn[16];
    formatDownAndDistance(game, downAndDistance, sizeof(downAndDistance));
    formatBallOn(game.world().lineOfScrimmageX(), ballOn, sizeof(ballOn));

    char buffer[192];
    std::snprintf(buffer, sizeof(buffer), "%s  |  %s  |  Ball on %s  |  %s %d - %d %s  |  :%02d",
                  game.currentPlay().name.c_str(), downAndDistance, ballOn, theme::kHomeName,
                  game.drive().homeScore(), game.drive().awayScore(), theme::kAwayName,
                  playClockSeconds);
    const int width = MeasureText(buffer, 20);
    const int left = static_cast<int>(topBarCenterX()) - width / 2;
    DrawRectangle(left - 12, 8, width + 24, 32, theme::panel(0.6f));
    DrawText(buffer, left, 14, 20, theme::kLineWhite);
}

void drawResultBanner(const PlayOutcome& outcome) {
    const int screenWidth = GetScreenWidth();
    const int y = GetScreenHeight() / 3;
    DrawRectangle(0, y - 40, screenWidth, 80, theme::panel(0.7f));

    char buffer[64];
    if (outcome.result == PlayResult::Incomplete) {
        std::snprintf(buffer, sizeof(buffer), "INCOMPLETE - no gain");
    } else {
        std::snprintf(buffer, sizeof(buffer), "%s  %+.1f yards", resultText(outcome.result),
                      static_cast<double>(outcome.yardsGained));
    }
    drawCenteredText(buffer, Vector2{static_cast<float>(screenWidth) / 2.0f, static_cast<float>(y)},
                     40, theme::kLineWhite);
}

// ---------------------------------------------------------------- broadcast scorebug (polish on)

// Text in the TTF font, aligned inside a box: align 0 = left, 0.5 = center, 1 = right.
void drawBoxText(const Font& font, const char* text, Rectangle box, float size, float align,
                 Color color) {
    const float spacing = 1.0f;
    const Vector2 measured = MeasureTextEx(font, text, size, spacing);
    const float padding = 12.0f;
    const float x = box.x + padding + (box.width - 2.0f * padding - measured.x) * align;
    const float y = box.y + (box.height - measured.y) / 2.0f;
    DrawTextEx(font, text, Vector2{x, y}, size, spacing, color);
}

void drawTeamSegment(const Font& font, Rectangle box, Color teamColor, const char* name,
                     int score) {
    DrawRectangleRec(box, teamColor);
    // Subtle top-to-bottom shading so the block doesn't look flat.
    DrawRectangleGradientV(static_cast<int>(box.x), static_cast<int>(box.y),
                           static_cast<int>(box.width), static_cast<int>(box.height),
                           Fade(theme::kLineWhite, 0.12f), Fade(theme::kUiDark, 0.18f));
    char scoreText[8];
    std::snprintf(scoreText, sizeof(scoreText), "%d", score);
    drawBoxText(font, name, box, 28.0f, 0.0f, theme::kLineWhite);
    drawBoxText(font, scoreText, box, 36.0f, 1.0f, theme::kLineWhite);
}

void drawScorebug(const Game& game, const RenderContext& context, int playClockSeconds) {
    const Font& font = context.assets.font();
    constexpr float kHeight = kScorebugHeight;
    constexpr float kTeamWidth = 175.0f;
    constexpr float kDownWidth = 170.0f;
    constexpr float kBallOnWidth = 120.0f;
    constexpr float kClockWidth = 80.0f;
    constexpr float kTotalWidth = 2.0f * kTeamWidth + kDownWidth + kBallOnWidth + kClockWidth;

    const float x0 = (static_cast<float>(GetScreenWidth()) - kTotalWidth) / 2.0f;
    const float y =
        static_cast<float>(GetScreenHeight() - kHelpBarHeight - kScorebugMargin) - kHeight;

    // Drop shadow, then the dark base the segments sit on.
    DrawRectangleRec(Rectangle{x0 + 4.0f, y + 4.0f, kTotalWidth, kHeight}, theme::shadow(0.5f));
    DrawRectangleRec(Rectangle{x0, y, kTotalWidth, kHeight}, theme::panel(0.92f));

    float x = x0;
    const Drive& drive = game.drive();
    drawTeamSegment(font, Rectangle{x, y, kTeamWidth, kHeight}, theme::kHome, theme::kHomeName,
                    drive.homeScore());
    x += kTeamWidth;
    drawTeamSegment(font, Rectangle{x, y, kTeamWidth, kHeight}, theme::kAway, theme::kAwayName,
                    drive.awayScore());
    x += kTeamWidth;

    char text[24];
    formatDownAndDistance(game, text, sizeof(text));
    drawBoxText(font, text, Rectangle{x, y, kDownWidth, kHeight}, 30.0f, 0.5f, theme::kAccent);
    x += kDownWidth;

    formatBallOn(game.world().lineOfScrimmageX(), text, sizeof(text));
    drawBoxText(font, text, Rectangle{x, y, kBallOnWidth, kHeight}, 26.0f, 0.5f, theme::kLineWhite);
    x += kBallOnWidth;

    // Separator lines between the info segments.
    for (const float sx : {x0 + 2.0f * kTeamWidth + kDownWidth, x}) {
        DrawLineEx(Vector2{sx, y + 8.0f}, Vector2{sx, y + kHeight - 8.0f}, 1.0f,
                   Fade(theme::kLineWhite, 0.25f));
    }

    std::snprintf(text, sizeof(text), ":%02d", playClockSeconds);
    const Color clockColor = playClockSeconds <= 5 ? theme::kAway : theme::kLineWhite;
    drawBoxText(font, text, Rectangle{x, y, kClockWidth, kHeight}, 30.0f, 0.5f, clockColor);

    // Thin accent rule across the top edge, a common broadcast detail.
    DrawRectangleRec(Rectangle{x0, y - 3.0f, kTotalWidth, 3.0f}, theme::kAccent);
}

void drawPlayCallTag(const Game& game, const RenderContext& context) {
    const Font& font = context.assets.font();
    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "%s  vs  %s", game.currentPlay().name.c_str(),
                  game.defense().name.c_str());
    const float size = 26.0f;
    const Vector2 measured = MeasureTextEx(font, buffer, size, 1.0f);
    const float x = topBarCenterX() - measured.x / 2.0f;
    DrawRectangleRec(Rectangle{x - 14.0f, 10.0f, measured.x + 28.0f, 36.0f}, theme::panel(0.75f));
    DrawRectangleRec(Rectangle{x - 14.0f, 10.0f, 4.0f, 36.0f}, theme::kAccent);
    DrawTextEx(font, buffer, Vector2{x, 10.0f + (36.0f - measured.y) / 2.0f}, size, 1.0f,
               theme::kLineWhite);
}

} // namespace

int hudBottomInset(bool polish) {
    return kHelpBarHeight + (polish ? kScorebugMargin + static_cast<int>(kScorebugHeight) : 0);
}

void drawHud(const Game& game, const RenderContext& context, int playClockSeconds) {
    const World& world = game.world();
    if (context.polish) {
        drawPlayCallTag(game, context);
        drawScorebug(game, context, playClockSeconds);
    } else {
        drawPlainTopBar(game, playClockSeconds);
        if (world.phase() == PlayPhase::Over) {
            drawResultBanner(world.outcome()); // polish uses the animated popup instead
        }
    }
    drawControlsHelp(world);
}

} // namespace gridiron::render
