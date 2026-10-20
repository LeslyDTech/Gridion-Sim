#include "Hud.h"

#include "Assets.h"
#include "BroadcastHud.h"
#include "DebugOverlay.h"
#include "Text.h"
#include "Theme.h"
#include "core/FieldGeometry.h"
#include "core/Game.h"

#include <raylib.h>

#include <cstdio>

namespace gridiron::render {
namespace {

constexpr int kHelpBarHeight = 32; // plain mode (F2 off)

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

// `font` is the TTF with polish on, or nullptr for raylib's default font (plain mode).
void drawControlsHelp(const World& world, const Font* font) {
    const int screenWidth = GetScreenWidth();
    const int screenHeight = GetScreenHeight();
    DrawRectangle(0, screenHeight - kHelpBarHeight, screenWidth, kHelpBarHeight,
                  theme::panel(0.6f));
    const Vector2 center{static_cast<float>(screenWidth) / 2.0f,
                         static_cast<float>(screenHeight - kHelpBarHeight / 2)};
    if (font != nullptr) {
        text::drawCentered(*font, controlsHelp(world), center, 22.0f, theme::kLineWhite);
    } else {
        drawCenteredText(controlsHelp(world), center, 18, theme::kLineWhite);
    }
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

} // namespace

int hudBottomInset(bool polish) {
    return polish ? broadcast::bottomInset() : kHelpBarHeight;
}

void drawHud(const Game& game, const RenderContext& context) {
    const int playClockSeconds = game.playClockSeconds();
    const World& world = game.world();
    if (context.polish) {
        broadcast::drawPlayCallBanner(game, context.assets);
        broadcast::drawPlaySelectPanel(game, context.assets);
        broadcast::drawScorebug(game, context.assets);
        broadcast::drawControlsBar(world, context.assets);
        return;
    }
    drawPlainTopBar(game, playClockSeconds);
    if (world.phase() == PlayPhase::Over) {
        drawResultBanner(world.outcome()); // polish uses the animated popup instead
    }
    drawControlsHelp(world, nullptr);
}

} // namespace gridiron::render
