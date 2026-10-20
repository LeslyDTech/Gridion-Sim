#pragma once

#include <raylib.h>

// The single source of visual identity. Every color drawn on screen is one of these seven, or a
// faded/brightened variant of one (Fade, ColorBrightness), so the look stays coherent and a
// re-theme is a one-file change.
namespace gridiron::theme {

// --- Palette ---
inline constexpr Color kHome{29, 66, 160, 255}; // offense: royal blue
inline constexpr Color kAway{178, 34, 52, 255}; // defense: crimson
inline constexpr Color kGrassLight{58, 135, 64, 255};
inline constexpr Color kGrassDark{48, 117, 55, 255};
inline constexpr Color kLineWhite{242, 242, 236, 255};
inline constexpr Color kAccent{255, 196, 37, 255}; // gold: ball carrier, routes, highlights
inline constexpr Color kUiDark{18, 22, 30, 255};   // panels, shadows, the football itself

// --- Team identity (fictional) ---
inline constexpr const char* kHomeName = "COMETS";
inline constexpr const char* kAwayName = "BLAZE";

// --- Derived helpers ---
inline Color panel(float opacity = 0.75f) { return Fade(kUiDark, opacity); }
inline Color shadow(float opacity = 0.35f) { return Fade(kUiDark, opacity); }
inline Color sidelineArea() { return ColorBrightness(kGrassDark, -0.35f); }

} // namespace gridiron::theme
