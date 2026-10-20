#include "BroadcastHud.h"

#include "Assets.h"
#include "Easing.h"
#include "Theme.h"
#include "UiDraw.h"
#include "core/FieldGeometry.h"
#include "core/Game.h"

#include <raylib.h>

#include <algorithm>
#include <cmath>
#include <cstdio>

namespace gridiron::render::broadcast {
namespace {

using namespace theme::hud;

using ui::drawText;
using ui::drawTextInBox;
using ui::fillClippedToRounded;
using ui::fillRounded;
using ui::fontFor;
using ui::measure;
using ui::pixelSize;
using ui::screenHeight;
using ui::screenWidth;
using ui::toUpper;

// ---------------------------------------------------------------- game-state formatting

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
                      drive.yardsToGo(game.lineOfScrimmageX()));
    }
}

// "OWN 25" / "OPP 40" / "50"
void formatFieldPosition(float x, char* buffer, std::size_t size) {
    const int line = field::yardLineNumber(x);
    if (line == 50) {
        std::snprintf(buffer, size, "50");
    } else {
        std::snprintf(buffer, size, "%s %d", x < field::kLength / 2.0f ? "OWN" : "OPP", line);
    }
}

// "+12 YDS", "-3 YDS", "NO GAIN", "TOUCHDOWN", ...
void formatYardage(const PlayOutcome& outcome, char* buffer, std::size_t size) {
    switch (outcome.result) {
    case PlayResult::Touchdown:
        std::snprintf(buffer, size, "TOUCHDOWN");
        return;
    case PlayResult::Safety:
        std::snprintf(buffer, size, "SAFETY");
        return;
    case PlayResult::Incomplete:
        std::snprintf(buffer, size, "INCOMPLETE");
        return;
    case PlayResult::Tackled:
    case PlayResult::OutOfBounds:
    case PlayResult::None:
        break;
    }
    const long yards = std::lround(outcome.yardsGained);
    if (yards == 0) {
        std::snprintf(buffer, size, "NO GAIN");
    } else {
        std::snprintf(buffer, size, "%+ld YDS", yards);
    }
}

// ---------------------------------------------------------------- ControlsBar content

// One run of the instructions line: key names are full white, the rest 70% white.
struct InstructionRun {
    const char* text;
    bool isKey;
};

#define GRIDIRON_DOT " \xC2\xB7 " // " · " separator (UTF-8 middle dot)

constexpr InstructionRun kPreSnapInstructions[] = {
    {"SPACE / (A)", true}, {" SNAP" GRIDIRON_DOT, false},
    {"TAB", true},         {" NEXT PLAY" GRIDIRON_DOT, false},
    {"F1", true},          {" DEBUG" GRIDIRON_DOT, false},
    {"F2", true},          {" POLISH", false},
};
constexpr InstructionRun kPassInstructions[] = {
    {"1-5", true},
    {" THROW" GRIDIRON_DOT, false},
    {"ARROWS / WASD", true},
    {" SCRAMBLE", false},
};
constexpr InstructionRun kRunInstructions[] = {{"ARROWS / WASD", true}, {" RUN", false}};
constexpr InstructionRun kResultInstructions[] = {{"SPACE", true}, {" NEXT PLAY", false}};

#undef GRIDIRON_DOT

struct Instructions {
    const InstructionRun* runs;
    std::size_t count;
};

template <std::size_t N>
constexpr Instructions instructions(const InstructionRun (&runs)[N]) {
    return {runs, N};
}

Instructions instructionsFor(const World& world) {
    switch (world.phase()) {
    case PlayPhase::PreSnap:
        return instructions(kPreSnapInstructions);
    case PlayPhase::Live: {
        const Ball& ball = world.ball();
        const bool qbHasBall =
            ball.state == BallState::Held &&
            world.players()[static_cast<std::size_t>(ball.carrierId)].role == Role::QB;
        return qbHasBall ? instructions(kPassInstructions) : instructions(kRunInstructions);
    }
    case PlayPhase::Over:
        break;
    }
    return instructions(kResultInstructions);
}

// ---------------------------------------------------------------- YardsGainedPopup animation

struct PopupAnimation {
    float scale;
    float opacity;
};

// Scale in with an ease-out, hold, then fade. Driven by Game's result timer (core state), so it
// needs no state of its own.
PopupAnimation popupAnimation(float secondsSinceEnd) {
    const float scaleIn = kYardsGainedPopupScaleInSeconds;
    const float fadeStart = scaleIn + kYardsGainedPopupHoldSeconds;
    const float scaleT = easing::outCubic(secondsSinceEnd / scaleIn);
    const float scale = kYardsGainedPopupStartScale + (1.0f - kYardsGainedPopupStartScale) * scaleT;
    const float opacity =
        1.0f - easing::clamp01((secondsSinceEnd - fadeStart) / kYardsGainedPopupFadeSeconds);
    return {scale, opacity};
}

// Text with a solid outline: the text drawn in the outline color at 8 offsets around a circle,
// then the text itself on top.
void drawOutlinedText(const Font& font, const char* text, Vector2 topLeft, float size,
                      float outline, Color fill, Color outlineColor) {
    constexpr float kDiagonal = 0.70710678f;
    const Vector2 offsets[8] = {{1.0f, 0.0f},
                                {-1.0f, 0.0f},
                                {0.0f, 1.0f},
                                {0.0f, -1.0f},
                                {kDiagonal, kDiagonal},
                                {-kDiagonal, kDiagonal},
                                {kDiagonal, -kDiagonal},
                                {-kDiagonal, -kDiagonal}};
    for (const Vector2 offset : offsets) {
        DrawTextEx(font, text,
                   Vector2{topLeft.x + offset.x * outline, topLeft.y + offset.y * outline}, size,
                   0.0f, outlineColor);
    }
    DrawTextEx(font, text, topLeft, size, 0.0f, fill);
}

} // namespace

// ================================================================ PlayCallBanner

void drawPlayCallBanner(const Game& game, const Assets& assets) {
    char playName[64];
    char defenseName[64];
    toUpper(game.currentPlay().name.c_str(), playName, sizeof(playName));
    toUpper(game.defense().name.c_str(), defenseName, sizeof(defenseName));
    char label[160];
    std::snprintf(label, sizeof(label), "%s  VS  %s", playName, defenseName);

    // Width hugs the label: accent + padding + text + padding.
    const Vector2 textSize = measure(assets, kPlayCallBannerLabel, label);
    const float width =
        kPlayCallBannerAccentWidth + 2.0f * kPlayCallBannerContentPaddingX + std::ceil(textSize.x);
    const Rectangle banner{std::round((screenWidth() - width) / 2.0f), kPlayCallBannerTop, width,
                           kPlayCallBannerHeight};

    fillRounded(banner, kPlayCallBannerCornerRadius, kPlayCallBannerBackground);
    fillClippedToRounded(banner, kPlayCallBannerCornerRadius,
                         Rectangle{banner.x, banner.y, kPlayCallBannerAccentWidth, banner.height},
                         kPlayCallBannerAccent);

    const Rectangle content{banner.x + kPlayCallBannerAccentWidth, banner.y,
                            width - kPlayCallBannerAccentWidth, banner.height};
    drawTextInBox(assets, kPlayCallBannerLabel, label, content, kPlayCallBannerContentPaddingX,
                  0.0f);
}

// ================================================================ PlaySelectPanel/PreSnap

void HudAnimator::update(const Game& game, float frameSeconds) {
    const float target = game.selectingPlay() ? 1.0f : 0.0f;
    const float step = frameSeconds / kPlaySelectPanelSlideSeconds;
    m_panelShown = target > m_panelShown ? std::min(target, m_panelShown + step)
                                         : std::max(target, m_panelShown - step);
}

void drawPlaySelectPanel(const Game& game, const Assets& assets, float shown) {
    if (shown <= 0.0f) {
        return;
    }
    const auto& playbook = game.playbook();
    const int rows = std::min(kPlaySelectPanelRows, static_cast<int>(playbook.size()));
    const float height =
        kPlaySelectPanelHeaderHeight + static_cast<float>(rows) * kPlaySelectPanelRowHeight;
    // Slide: at shown = 0 the panel sits just past the right edge of the screen. Ease-out on the
    // way in, ease-in on the way out (the same curve, mirrored).
    const float slide = 1.0f - easing::outCubic(shown);
    const float offscreen = kPlaySelectPanelRight + kPlaySelectPanelWidth;
    const Rectangle panel{std::round(screenWidth() - kPlaySelectPanelRight - kPlaySelectPanelWidth +
                                     slide * offscreen),
                          kPlaySelectPanelTop, kPlaySelectPanelWidth, height};

    fillRounded(panel, kPlaySelectPanelCornerRadius, kPlaySelectPanelBackground);
    drawTextInBox(assets, kPlaySelectPanelTitle, kPlaySelectPanelTitleText,
                  Rectangle{panel.x, panel.y, panel.width, kPlaySelectPanelHeaderHeight},
                  kPlaySelectPanelHeaderPaddingX, 0.0f);

    for (int i = 0; i < rows; ++i) {
        const Rectangle row{panel.x,
                            panel.y + kPlaySelectPanelHeaderHeight +
                                static_cast<float>(i) * kPlaySelectPanelRowHeight,
                            panel.width, kPlaySelectPanelRowHeight};
        if (static_cast<std::size_t>(i) == game.currentPlayIndex()) {
            fillClippedToRounded(panel, kPlaySelectPanelCornerRadius, row,
                                 kPlaySelectPanelSelectedFill);
            fillClippedToRounded(
                panel, kPlaySelectPanelCornerRadius,
                Rectangle{row.x, row.y, kPlaySelectPanelSelectedBorderWidth, row.height},
                kPlaySelectPanelSelectedBorder);
        }

        // Name fills the row up to the key hint (Figma: name box 224 wide, hint 24 wide).
        const Rectangle nameBox{
            row.x, row.y, row.width - kPlaySelectPanelKeyHintSize - kPlaySelectPanelRowPaddingX,
            row.height};
        drawTextInBox(assets, kPlaySelectPanelPlayName,
                      playbook[static_cast<std::size_t>(i)].name.c_str(), nameBox,
                      kPlaySelectPanelRowPaddingX, 0.0f);

        const Rectangle keyHint{row.x + row.width - kPlaySelectPanelRowPaddingX -
                                    kPlaySelectPanelKeyHintSize,
                                row.y + (row.height - kPlaySelectPanelKeyHintSize) / 2.0f,
                                kPlaySelectPanelKeyHintSize, kPlaySelectPanelKeyHintSize};
        char key[4];
        std::snprintf(key, sizeof(key), "%d", i + 1);
        drawTextInBox(assets, kPlaySelectPanelKey, key, keyHint, 0.0f, 0.5f);
    }
}

// ================================================================ YardsGainedPopup

void drawYardsGainedPopup(const Game& game, const Assets& assets) {
    if (game.world().phase() != PlayPhase::Over) {
        return;
    }
    const PopupAnimation anim = popupAnimation(game.secondsSincePlayEnded());
    if (anim.opacity <= 0.0f) {
        return;
    }

    // Layout at scale 1 around the screen center, then scale every coordinate about the center.
    const Vector2 center{screenWidth() / 2.0f, screenHeight() / 2.0f};
    const float top = center.y - kYardsGainedPopupHeight / 2.0f;
    auto scaled = [&](float x, float y) {
        return Vector2{center.x + (x - center.x) * anim.scale,
                       center.y + (y - center.y) * anim.scale};
    };

    // YardsGainedPopup/ValueContainer: the yardage, centered.
    char yardage[24];
    formatYardage(game.world().outcome(), yardage, sizeof(yardage));
    const Vector2 valueSize = measure(assets, kYardsGainedPopupYardage, yardage);
    const Vector2 valueTopLeft =
        scaled(center.x - valueSize.x / 2.0f,
               top + (kYardsGainedPopupValueContainerHeight - valueSize.y) / 2.0f);
    drawOutlinedText(fontFor(assets, kYardsGainedPopupYardage), yardage, valueTopLeft,
                     pixelSize(kYardsGainedPopupYardage) * anim.scale,
                     kYardsGainedPopupYardageOutline * anim.scale,
                     Fade(kYardsGainedPopupYardage.color, anim.opacity),
                     Fade(kYardsGainedPopupYardageOutlineColor, anim.opacity));

    // YardsGainedPopup/TagContainer: the FIRST DOWN tag, only when the play earned one.
    if (game.lastDriveEvent() != DriveEvent::FirstDown) {
        return;
    }
    const Vector2 labelSize =
        measure(assets, kYardsGainedPopupFirstDownLabel, kYardsGainedPopupFirstDownText);
    const float tagWidth = labelSize.x + 2.0f * kYardsGainedPopupFirstDownPaddingX;
    const float tagHeight = labelSize.y + 2.0f * kYardsGainedPopupFirstDownPaddingY;
    const float tagContainerTop =
        top + kYardsGainedPopupValueContainerHeight + kYardsGainedPopupGap;
    const float tagTop = tagContainerTop + (kYardsGainedPopupTagContainerHeight - tagHeight) / 2.0f;
    const Vector2 tagTopLeft = scaled(center.x - tagWidth / 2.0f, tagTop);
    fillRounded(
        Rectangle{tagTopLeft.x, tagTopLeft.y, tagWidth * anim.scale, tagHeight * anim.scale},
        kYardsGainedPopupFirstDownCornerRadius * anim.scale,
        Fade(kYardsGainedPopupFirstDownFill, anim.opacity));
    const Vector2 labelTopLeft =
        scaled(center.x - labelSize.x / 2.0f, tagTop + kYardsGainedPopupFirstDownPaddingY);
    DrawTextEx(fontFor(assets, kYardsGainedPopupFirstDownLabel), kYardsGainedPopupFirstDownText,
               labelTopLeft, pixelSize(kYardsGainedPopupFirstDownLabel) * anim.scale, 0.0f,
               Fade(kYardsGainedPopupFirstDownLabel.color, anim.opacity));
}

// ================================================================ Scorebug

void drawScorebug(const Game& game, const Assets& assets) {
    const Rectangle bug{std::round((screenWidth() - kScorebugWidth) / 2.0f),
                        screenHeight() - kScorebugBottom - kScorebugHeight, kScorebugWidth,
                        kScorebugHeight};
    fillRounded(bug, kScorebugCornerRadius, kScorebugBackground);

    const Drive& drive = game.drive();
    char text[24];
    float x = bug.x;

    // Scorebug/HomeTeam: touches the rounded left corners, so it is clipped to the bug's shape.
    const Rectangle home{x, bug.y, kScorebugTeamWidth, bug.height};
    fillClippedToRounded(bug, kScorebugCornerRadius, home, kScorebugHomeTeamFill);
    drawTextInBox(assets, kScorebugTeamName, theme::kHomeName, home, kScorebugSegmentPaddingX,
                  0.0f);
    std::snprintf(text, sizeof(text), "%d", drive.homeScore());
    drawTextInBox(assets, kScorebugTeamScore, text, home, kScorebugSegmentPaddingX, 1.0f);
    x += kScorebugTeamWidth;

    // Scorebug/AwayTeam
    const Rectangle away{x, bug.y, kScorebugTeamWidth, bug.height};
    DrawRectangleRec(away, kScorebugAwayTeamFill);
    drawTextInBox(assets, kScorebugTeamName, theme::kAwayName, away, kScorebugSegmentPaddingX,
                  0.0f);
    std::snprintf(text, sizeof(text), "%d", drive.awayScore());
    drawTextInBox(assets, kScorebugTeamScore, text, away, kScorebugSegmentPaddingX, 1.0f);
    x += kScorebugTeamWidth;

    // Scorebug/DownDistance
    formatDownAndDistance(game, text, sizeof(text));
    drawTextInBox(assets, kScorebugDownDistanceLabel, text,
                  Rectangle{x, bug.y, kScorebugDownDistanceWidth, bug.height},
                  kScorebugSegmentPaddingX, 0.0f);
    x += kScorebugDownDistanceWidth;

    // Scorebug/Divider/DownDistance
    DrawRectangleRec(Rectangle{x, bug.y, kScorebugDividerWidth, bug.height}, kScorebugDivider);
    x += kScorebugDividerWidth;

    // Scorebug/FieldPosition: the line of scrimmage of the upcoming (or current) snap.
    formatFieldPosition(game.lineOfScrimmageX(), text, sizeof(text));
    drawTextInBox(assets, kScorebugFieldPositionLabel, text,
                  Rectangle{x, bug.y, kScorebugFieldPositionWidth, bug.height},
                  kScorebugSegmentPaddingX, 0.0f);
    x += kScorebugFieldPositionWidth;

    // Scorebug/Divider/PlayClock
    DrawRectangleRec(Rectangle{x, bug.y, kScorebugDividerWidth, bug.height}, kScorebugDivider);
    x += kScorebugDividerWidth;

    // Scorebug/PlayClock
    std::snprintf(text, sizeof(text), ":%02d", game.playClockSeconds());
    drawTextInBox(assets, kScorebugPlayClockValue, text,
                  Rectangle{x, bug.y, kScorebugPlayClockWidth, bug.height},
                  kScorebugSegmentPaddingX, 0.0f);

    // Scorebug/TopAccent: drawn last, over the segments, clipped to the rounded top corners.
    fillClippedToRounded(bug, kScorebugCornerRadius,
                         Rectangle{bug.x, bug.y, bug.width, kScorebugTopAccentHeight},
                         kScorebugTopAccent);
}

// ================================================================ ControlsBar

void drawControlsBar(const World& world, const Assets& assets) {
    const Rectangle bar{0.0f, screenHeight() - kControlsBarHeight, screenWidth(),
                        kControlsBarHeight};
    DrawRectangleRec(bar, kControlsBarBackground);

    const Instructions line = instructionsFor(world);
    float totalWidth = 0.0f;
    for (std::size_t i = 0; i < line.count; ++i) {
        totalWidth += measure(assets, kControlsBarInstructions, line.runs[i].text).x;
    }
    float x = std::round((bar.width - totalWidth) / 2.0f);
    const float y = bar.y + (bar.height - pixelSize(kControlsBarInstructions)) / 2.0f;
    for (std::size_t i = 0; i < line.count; ++i) {
        const InstructionRun& run = line.runs[i];
        const Color color =
            run.isKey ? kControlsBarInstructionsKey : kControlsBarInstructions.color;
        drawText(assets, kControlsBarInstructions, run.text, Vector2{x, y}, color);
        x += measure(assets, kControlsBarInstructions, run.text).x;
    }
}

int topInset() { return static_cast<int>(kPlayCallBannerTop + kPlayCallBannerHeight); }
int bottomInset() { return static_cast<int>(kScorebugBottom + kScorebugHeight); }

} // namespace gridiron::render::broadcast
