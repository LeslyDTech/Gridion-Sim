#include "Hud.h"

#include "Theme.h"
#include "core/FieldGeometry.h"
#include "core/Game.h"

#include <raylib.h>

#include <cstdio>

namespace gridiron::render {
namespace {

void drawCenteredText(const char* text, Vector2 center, int fontSize, Color color) {
    const int width = MeasureText(text, fontSize);
    DrawText(text, static_cast<int>(center.x) - width / 2,
             static_cast<int>(center.y) - fontSize / 2, fontSize, color);
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

// "Own 25" / "Opp 40" / "50".
void formatBallOn(float x, char* buffer, std::size_t size) {
    const int line = field::yardLineNumber(x);
    if (line == 50) {
        std::snprintf(buffer, size, "50");
    } else {
        std::snprintf(buffer, size, "%s %d", x < field::kLength / 2.0f ? "Own" : "Opp", line);
    }
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

void drawPlayCall(const Game& game) {
    char ballOn[16];
    formatBallOn(game.world().lineOfScrimmageX(), ballOn, sizeof(ballOn));
    char buffer[128];
    std::snprintf(buffer, sizeof(buffer), "%s  vs  %s   |   Ball on %s",
                  game.currentPlay().name.c_str(), game.defense().name.c_str(), ballOn);
    const int screenWidth = GetScreenWidth();
    const int width = MeasureText(buffer, 20);
    DrawRectangle(screenWidth / 2 - width / 2 - 12, 8, width + 24, 32, theme::panel(0.6f));
    DrawText(buffer, screenWidth / 2 - width / 2, 14, 20, theme::kLineWhite);
}

void drawControlsHelp(const World& world) {
    const int screenWidth = GetScreenWidth();
    const int screenHeight = GetScreenHeight();
    DrawRectangle(0, screenHeight - 32, screenWidth, 32, theme::panel(0.6f));
    drawCenteredText(
        controlsHelp(world),
        Vector2{static_cast<float>(screenWidth) / 2.0f, static_cast<float>(screenHeight) - 16.0f},
        18, theme::kLineWhite);
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

void drawHud(const Game& game, const RenderContext& context) {
    const World& world = game.world();
    drawPlayCall(game);
    drawControlsHelp(world);
    // With polish on, the animated "+N YDS" popup (Effects) replaces the plain banner.
    if (!context.polish && world.phase() == PlayPhase::Over) {
        drawResultBanner(world.outcome());
    }
}

} // namespace gridiron::render
