#pragma once

#include <raylib.h>

// The single source of visual identity.
//
// Field and players: the seven palette colors below (or faded/brightened variants of them).
// Broadcast HUD: the `hud` section, transcribed from the Figma frame
// "Football Broadcast HUD / 1280x720" (node 2:286). Every constant there is named after the Figma
// layer it comes from, so a design change is a one-line diff here.
namespace gridiron::theme {

// --- Palette ---
// Team and grass colors are sampled from the Kenney Sports Pack art (assets/), so code-drawn
// shapes, the plain (F2 off) mode and the sprites all agree.
inline constexpr Color kHome{65, 159, 221, 255};       // offense: sprite jersey blue
inline constexpr Color kAway{232, 106, 23, 255};       // defense: sprite jersey orange
inline constexpr Color kGrassLight{49, 217, 120, 255}; // the two stripes of the grass tile
inline constexpr Color kGrassDark{46, 204, 113, 255};
inline constexpr Color kLineWhite{242, 242, 236, 255};
// Yellow is reserved for the first-down line, the way broadcasts use it. Nothing else may use it.
inline constexpr Color kFirstDownYellow{255, 214, 10, 255};
inline constexpr Color kUiDark{18, 22, 30, 255}; // panels, shadows

// --- Team identity (fictional) ---
inline constexpr const char* kHomeName = "COMETS";
inline constexpr const char* kAwayName = "BLAZE";

// --- Derived helpers ---
inline Color panel(float opacity = 0.75f) { return Fade(kUiDark, opacity); }
inline Color shadow(float opacity = 0.35f) { return Fade(kUiDark, opacity); }
inline Color sidelineArea() { return ColorBrightness(kGrassDark, -0.35f); }

} // namespace gridiron::theme

// =================================================================================================
// Broadcast HUD (Figma node 2:286)
//
// Units are Figma pixels for a 1280x720 frame. Sizes and paddings are used as-is at any window
// size; positions are offsets from the screen edge each group is anchored to (see Hud.cpp).
// Font sizes are Figma font sizes: Hud.cpp converts them to raylib sizes (x1.2 for Barlow
// Condensed, whose ascender-descender is 1.2 em).
// =================================================================================================
namespace gridiron::theme::hud {

enum class Weight { Bold, SemiBold }; // Barlow Condensed Bold / SemiBold

struct TextStyle {
    Weight weight;
    float size; // Figma font size in px
    Color color;
};

// --- Shared Figma colors ---
inline constexpr Color kWhite{255, 255, 255, 255};
inline constexpr Color kInk{20, 22, 28, 255};            // #14161C
inline constexpr Color kPanel{20, 22, 28, 230};          // rgba(20,22,28,0.9)
inline constexpr Color kAccentYellow{245, 197, 24, 255}; // #F5C518
inline constexpr Color kHomeBlue{74, 144, 217, 255};     // #4A90D9
inline constexpr Color kAwayOrange{232, 119, 46, 255};   // #E8772E

// --- PlayCallBanner (anchored top center) ---
inline constexpr float kPlayCallBannerTop = 8.0f;
inline constexpr float kPlayCallBannerHeight = 40.0f;
inline constexpr float kPlayCallBannerCornerRadius = 4.0f;
inline constexpr Color kPlayCallBannerBackground = kPanel;
inline constexpr float kPlayCallBannerAccentWidth = 4.0f;
inline constexpr Color kPlayCallBannerAccent = kAccentYellow;
inline constexpr float kPlayCallBannerContentPaddingX = 16.0f;
inline constexpr TextStyle kPlayCallBannerLabel{Weight::Bold, 20.0f, kWhite}; // uppercase

// --- PlaySelectPanel/PreSnap (anchored top right) ---
inline constexpr float kPlaySelectPanelRight = 16.0f; // 1280 - (984 + 280)
inline constexpr float kPlaySelectPanelTop = 72.0f;
inline constexpr float kPlaySelectPanelWidth = 280.0f;
inline constexpr float kPlaySelectPanelCornerRadius = 4.0f;
inline constexpr Color kPlaySelectPanelBackground = kPanel;
inline constexpr float kPlaySelectPanelHeaderHeight = 40.0f;
inline constexpr float kPlaySelectPanelHeaderPaddingX = 16.0f;
inline constexpr TextStyle kPlaySelectPanelTitle{Weight::SemiBold, 14.0f, kWhite};
inline constexpr const char* kPlaySelectPanelTitleText =
    "OFFENSE \xC2\xB7 PLAYBOOK"; // U+00B7 middle dot, UTF-8
inline constexpr float kPlaySelectPanelRowHeight = 44.0f;
inline constexpr float kPlaySelectPanelRowPaddingX = 16.0f;
inline constexpr TextStyle kPlaySelectPanelPlayName{Weight::SemiBold, 16.0f, kWhite};
inline constexpr float kPlaySelectPanelKeyHintSize = 24.0f;
inline constexpr float kPlaySelectPanelKeyHintCornerRadius = 4.0f;
inline constexpr TextStyle kPlaySelectPanelKey{Weight::Bold, 14.0f, kWhite};
inline constexpr Color kPlaySelectPanelSelectedFill = kHomeBlue;
inline constexpr float kPlaySelectPanelSelectedBorderWidth = 3.0f;
inline constexpr Color kPlaySelectPanelSelectedBorder = kWhite;
inline constexpr int kPlaySelectPanelRows = 4;
// Slide out to the right once a play is chosen, back in when selection reopens (not in the
// static frame; from the HUD spec).
inline constexpr float kPlaySelectPanelSlideSeconds = 0.25f;

// --- YardsGainedPopup/State=FirstDown (anchored to the screen center) ---
inline constexpr float kYardsGainedPopupWidth = 240.0f;
inline constexpr float kYardsGainedPopupHeight = 112.0f;
inline constexpr float kYardsGainedPopupGap = 8.0f;
inline constexpr float kYardsGainedPopupValueContainerHeight = 80.0f;
inline constexpr TextStyle kYardsGainedPopupYardage{Weight::Bold, 64.0f, kWhite};
inline constexpr float kYardsGainedPopupYardageOutline = 2.0f; // measured from the Figma render
inline constexpr Color kYardsGainedPopupYardageOutlineColor = kInk;
inline constexpr float kYardsGainedPopupTagContainerHeight = 24.0f;
inline constexpr float kYardsGainedPopupFirstDownPaddingX = 16.0f;
inline constexpr float kYardsGainedPopupFirstDownPaddingY = 4.0f;
inline constexpr float kYardsGainedPopupFirstDownCornerRadius = 4.0f;
inline constexpr Color kYardsGainedPopupFirstDownFill = kAccentYellow;
inline constexpr TextStyle kYardsGainedPopupFirstDownLabel{Weight::SemiBold, 14.0f, kInk};
inline constexpr const char* kYardsGainedPopupFirstDownText = "FIRST DOWN";
// Animation (not in the static frame; from the HUD spec): scale in, hold ~1.5 s, fade.
inline constexpr float kYardsGainedPopupScaleInSeconds = 0.25f;
inline constexpr float kYardsGainedPopupStartScale = 0.6f;
inline constexpr float kYardsGainedPopupHoldSeconds = 1.5f;
inline constexpr float kYardsGainedPopupFadeSeconds = 0.4f;

// --- Scorebug (anchored bottom center) ---
inline constexpr float kScorebugWidth = 720.0f;
inline constexpr float kScorebugHeight = 48.0f;
inline constexpr float kScorebugBottom = 44.0f; // 720 - (628 + 48)
inline constexpr float kScorebugCornerRadius = 4.0f;
inline constexpr Color kScorebugBackground = kPanel;
inline constexpr float kScorebugTopAccentHeight = 3.0f;
inline constexpr Color kScorebugTopAccent = kAccentYellow;
inline constexpr float kScorebugSegmentPaddingX = 16.0f;
inline constexpr float kScorebugTeamWidth = 200.0f; // HomeTeam and AwayTeam
inline constexpr Color kScorebugHomeTeamFill = kHomeBlue;
inline constexpr Color kScorebugAwayTeamFill = kAwayOrange;
inline constexpr TextStyle kScorebugTeamName{Weight::Bold, 28.0f, kWhite};
inline constexpr TextStyle kScorebugTeamScore{Weight::Bold, 28.0f, kWhite};
inline constexpr float kScorebugDownDistanceWidth = 128.0f;
inline constexpr TextStyle kScorebugDownDistanceLabel{Weight::Bold, 18.0f, kAccentYellow};
inline constexpr float kScorebugDividerWidth = 1.0f;
inline constexpr Color kScorebugDivider = kWhite;
inline constexpr float kScorebugFieldPositionWidth = 96.0f;
inline constexpr TextStyle kScorebugFieldPositionLabel{Weight::SemiBold, 16.0f, kWhite};
inline constexpr float kScorebugPlayClockWidth = 94.0f;
inline constexpr TextStyle kScorebugPlayClockValue{Weight::Bold, 18.0f, kWhite};

// --- ControlsBar (anchored bottom, full width) ---
inline constexpr float kControlsBarHeight = 28.0f;
inline constexpr Color kControlsBarBackground{20, 22, 28, 204}; // rgba(20,22,28,0.8)
inline constexpr float kControlsBarContentPaddingX = 24.0f;
inline constexpr TextStyle kControlsBarInstructions{Weight::SemiBold, 14.0f, {255, 255, 255, 179}};
inline constexpr Color kControlsBarInstructionsKey = kWhite; // key names are full white

} // namespace gridiron::theme::hud
