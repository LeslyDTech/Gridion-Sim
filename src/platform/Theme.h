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
// Team colors are the player sprites' jersey colors (theme::sprite, from Figma) and the grass
// colors are sampled from the Kenney grass tile, so code-drawn shapes, the plain (F2 off) mode and
// the art all agree.
inline constexpr Color kHome{74, 144, 217, 255};       // offense: Comets/Primary #4A90D9
inline constexpr Color kAway{201, 62, 62, 255};        // defense: Blaze/Primary #C93E3E
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

// =================================================================================================
// Menus and full-screen scenes (Figma page "Page 2": Loading 8:344, Title 8:369, Main Menu 8:389,
// Pause 8:423). Same units and conventions as theme::hud; constants reuse the HUD's where the
// values match (panel dark, home blue, accent yellow, ink, selected-row style).
// =================================================================================================
namespace gridiron::theme::menu {

using hud::TextStyle;
using hud::Weight;

// Figma line heights (multiples of the font size); "normal" is 1.2 for Barlow Condensed.
inline constexpr float kLineHeightNone = 1.0f;
inline constexpr float kLineHeightNormal = 1.2f;

inline constexpr Color kWhite80{255, 255, 255, 204}; // rgba(255,255,255,0.8)
inline constexpr Color kWhite70{255, 255, 255, 179}; // rgba(255,255,255,0.7)
inline constexpr Color kWhite50{255, 255, 255, 128}; // rgba(255,255,255,0.5)

// --- Shared backdrop: "Field darkening overlay" + "Soft field vignette" ---
inline constexpr Color kFieldDarkeningOverlay{0, 0, 0, 102}; // rgba(0,0,0,0.4)
// Radial gradient centered on the screen, radii 704 x 432 at 1280 x 720 (55% / 60% of the
// screen): clear out to 45% of the radius, then to rgba(0,0,0,0.502) at the edge.
inline constexpr float kSoftFieldVignetteRadiusX = 704.0f / 1280.0f;
inline constexpr float kSoftFieldVignetteRadiusY = 432.0f / 720.0f;
inline constexpr float kSoftFieldVignetteClearUntil = 0.45f;
inline constexpr unsigned char kSoftFieldVignetteEdgeAlpha = 128;
// "Football field" placeholder: only drawn on the Loading screen, where the real field (attract
// mode) cannot run yet. The Figma green #2ECC71 is the darker grass-tile stripe.
inline constexpr Color kFootballField = kGrassDark;
inline constexpr float kVerticalYardLineSpacing = 128.0f;
inline constexpr Color kVerticalYardLine = hud::kWhite;
inline constexpr Color kSceneFade{0, 0, 0, 255}; // scene transitions fade through black

// --- "GRIDIRON SIM brand" ---
inline constexpr const char* kGridironSimLogoText = "GRIDIRON SIM";
inline constexpr TextStyle kGridironSimLogoLarge{Weight::Bold, 96.0f,
                                                 hud::kWhite}; // Loading, Title
inline constexpr TextStyle kGridironSimLogoSmall{Weight::Bold, 48.0f, hud::kWhite}; // Main Menu
inline constexpr float kBrandUnderlineHeight = 3.0f;
inline constexpr Color kBrandUnderline = hud::kAccentYellow;

// --- Loading (8:344) ---
inline constexpr float kLoadingBrandTop = 248.0f;
inline constexpr float kLoadingBrandUnderlineTop = 248.0f + 112.0f;
inline constexpr float kLoadingBrandUnderlineWidth = 96.0f;
inline constexpr float kAssetLoadingProgressTop = 408.0f;
inline constexpr float kAssetLoadingProgressWidth = 480.0f;
inline constexpr float kProgressTrackHeight = 12.0f;
inline constexpr Color kProgressTrack = hud::kInk;
inline constexpr Color kProgressFill = hud::kHomeBlue;
inline constexpr float kLoadingStatusOffsetY = 24.0f;
inline constexpr TextStyle kLoadingStatus{Weight::SemiBold, 14.0f, kWhite70}; // % and asset path
inline constexpr TextStyle kLoadingError{Weight::SemiBold, 14.0f, kAway};
inline constexpr float kGameplayTipPillTop = 624.0f;
inline constexpr float kGameplayTipPillHeight = 40.0f;
inline constexpr float kGameplayTipPillPaddingX = 24.0f;
inline constexpr float kGameplayTipPillCornerRadius = 4.0f;
inline constexpr Color kGameplayTipPillBackground = hud::kPanel;
inline constexpr TextStyle kReceiverTip{Weight::SemiBold, 18.0f, hud::kWhite};
inline constexpr float kLoadingMinimumSeconds = 1.0f; // spec: never just flash
inline constexpr float kTipSeconds = 2.5f;            // rotate the tip this often

// --- Title (8:369) ---
inline constexpr float kTitleBrandTop = 240.0f;
inline constexpr float kGameSubtitleTop = 360.0f;
inline constexpr const char* kGameSubtitleText = "TOP-DOWN FOOTBALL";
inline constexpr TextStyle kGameSubtitle{Weight::SemiBold, 18.0f, hud::kWhite};
inline constexpr float kPressStartTop = 440.0f;
inline constexpr const char* kPressStartText = "PRESS START";
inline constexpr TextStyle kPressStart{Weight::Bold, 28.0f, hud::kAccentYellow};
inline constexpr float kPressStartDimOpacity = 0.4f; // the "pulse dim state" layer
inline constexpr float kPressStartPulseSeconds = 1.0f;
inline constexpr const char* kBuildVersionText = "v0.1";
inline constexpr TextStyle kBuildVersion{Weight::SemiBold, 12.0f, kWhite50};
inline constexpr float kBuildVersionRight = 32.0f;  // 1280 - 1248
inline constexpr float kBuildVersionBottom = 48.0f; // 720 - 672

// --- Main Menu (8:389) ---
inline constexpr float kMainMenuBrandLeft = 64.0f;
inline constexpr float kMainMenuBrandTop = 56.0f;
inline constexpr float kMainMenuBrandUnderlineTop = 56.0f + 60.0f;
inline constexpr float kMainMenuBrandUnderlineWidth = 48.0f;
inline constexpr float kMainMenuOptionsLeft = 64.0f;
inline constexpr float kMainMenuOptionsTop = 200.0f;
inline constexpr float kMainMenuOptionsWidth = 360.0f;
// Rows: the Pause frame's tight spacing (no gap) is used for every menu, per the spec; the Main
// Menu frame itself has an 8 px gap.
inline constexpr float kMenuOptionHeight = 52.0f;
inline constexpr float kMenuOptionGap = 0.0f;
inline constexpr float kMenuOptionPaddingX = 24.0f;
inline constexpr TextStyle kOptionLabel{Weight::Bold, 24.0f, kWhite80};
inline constexpr Color kOptionLabelSelected = hud::kWhite;
inline constexpr Color kSelectedMenuOptionFill = hud::kPlaySelectPanelSelectedFill;
inline constexpr float kSelectedMenuOptionBorderWidth = hud::kPlaySelectPanelSelectedBorderWidth;
inline constexpr Color kSelectedMenuOptionBorder = hud::kPlaySelectPanelSelectedBorder;
inline constexpr float kSelectedOptionInformationRight = 128.0f; // 1280 - (752 + 400)
inline constexpr float kSelectedOptionInformationTop = 200.0f;
inline constexpr float kSelectedOptionInformationWidth = 400.0f;
inline constexpr float kSelectedOptionInformationHeight = 240.0f;
inline constexpr float kSelectedOptionInformationPadding = 32.0f;
inline constexpr float kSelectedOptionInformationGap = 16.0f;
inline constexpr float kSelectedOptionInformationCornerRadius = 4.0f;
inline constexpr Color kSelectedOptionInformationBackground = hud::kPanel;
inline constexpr float kInformationAccentWidth = 4.0f;
inline constexpr Color kInformationAccent = hud::kHomeBlue;
inline constexpr TextStyle kSelectedOptionTitle{Weight::Bold, 28.0f, hud::kWhite};
inline constexpr float kSelectedOptionTitleLineHeight = 1.1f;
inline constexpr TextStyle kSelectedOptionDescription{Weight::SemiBold, 24.0f, kWhite80};
inline constexpr float kSelectedOptionDescriptionLineHeight = 1.35f;
inline constexpr float kNavigationControlsBarHeight = 28.0f;
inline constexpr Color kNavigationControlsBarBackground = hud::kPanel;
inline constexpr TextStyle kNavigationInstructions{Weight::SemiBold, 14.0f, kWhite70};
// UTF-8: U+2191 U+2193 arrows, U+00B7 middle dots.
inline constexpr const char* kNavigationInstructionsText =
    "\xE2\x86\x91\xE2\x86\x93 NAVIGATE \xC2\xB7 ENTER / (A) SELECT \xC2\xB7 ESC / (B) BACK";

// --- Pause (8:423) ---
inline constexpr float kPauseMenuPanelSize = 400.0f;
inline constexpr float kPauseMenuPanelCenterOffsetY = -8.0f; // top 152 = centered 160 - 8
inline constexpr float kPauseMenuPanelCornerRadius = 4.0f;
inline constexpr Color kPauseMenuPanelBackground = hud::kPanel;
inline constexpr const char* kPausedTitleText = "PAUSED";
inline constexpr TextStyle kPausedTitle{Weight::Bold, 40.0f, hud::kWhite};
inline constexpr float kPausedTitleTop = 24.0f;
inline constexpr float kPausedTitleLineHeight = 1.2f;
inline constexpr float kPauseMenuOptionsLeft = 24.0f;
inline constexpr float kPauseMenuOptionsTop = 104.0f;
inline constexpr float kPauseMenuOptionsWidth = 352.0f;

} // namespace gridiron::theme::menu

// =================================================================================================
// Player sprites (Figma page "PlayerSprite", node 11:3)
//
// The game draws the frame "PlayerSprite / Transparent Export Atlas / 576 Base Tiles", exported
// to assets/sprites/players.png. Colors are the Figma variables of the same names.
// =================================================================================================
namespace gridiron::theme::sprite {

// Atlas layout: 96 x 96 tiles on a 120 px pitch, 16 per row, ordered
// team (Comets, Blaze) > position > pose > facing direction (down, up, left, right).
inline constexpr float kTileSize = 96.0f;
inline constexpr float kTilePitch = 120.0f;
inline constexpr int kTilesPerRow = 16;
// The movement anchor is the tile center (48, 48).
// Shoulder width of the idle body rig across the facing direction, measured from the art (the
// linemen rigs are a little wider, 72-76 px). The drawn size is scaled from this.
inline constexpr float kShoulderPixels = 68.0f;

// --- Shared ---
inline constexpr Color kOutline{17, 25, 36, 255}; // Shared/Outline #111924

// --- Skin ---
// The atlas is exported with Skin/Medium; the renderer swaps it for one of the five tones.
inline constexpr Color kSkinFair{242, 201, 165, 255};        // Skin/Fair #F2C9A5
inline constexpr Color kSkinLightMedium{221, 167, 121, 255}; // Skin/LightMedium #DDA779
inline constexpr Color kSkinMedium{185, 121, 80, 255};       // Skin/Medium #B97950
inline constexpr Color kSkinDeep{138, 81, 51, 255};          // Skin/Deep #8A5133
inline constexpr Color kSkinRichDeep{90, 50, 37, 255};       // Skin/RichDeep #5A3225
inline constexpr Color kSkinTones[] = {kSkinFair, kSkinLightMedium, kSkinMedium, kSkinDeep,
                                       kSkinRichDeep};

} // namespace gridiron::theme::sprite
