#pragma once

#include <raylib.h>

// The single source of visual identity. Every color drawn on screen is one of these seven, or a
// faded/brightened variant of one (Fade, ColorBrightness), so the look stays coherent and a
// re-theme is a one-file change.
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
