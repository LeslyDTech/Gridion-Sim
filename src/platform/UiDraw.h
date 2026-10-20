#pragma once

#include "Theme.h"

#include <raylib.h>

#include <cstddef>

namespace gridiron {

class Assets;

// Drawing helpers shared by the broadcast HUD and the menus, all working in Figma terms: text
// styles in Figma font sizes, boxes in pixels, corner radii in pixels.
namespace ui {

// Figma font size -> raylib font size. raylib scales a TTF so ascender - descender equals the
// requested size; CSS/Figma scale by the em square. Barlow Condensed's ascender - descender is
// 1.2 em (1000 - (-200) units at 1000 units/em), so a 20 px Figma label is a 24 px raylib label,
// and its text box lines up exactly with the Figma layer's box ("line-height: normal").
inline constexpr float kFigmaToRaylibFontScale = 1.2f;

float pixelSize(const theme::hud::TextStyle& style);
const Font& fontFor(const Assets& assets, const theme::hud::TextStyle& style);
Vector2 measure(const Assets& assets, const theme::hud::TextStyle& style, const char* text);

// Where the glyphs start for a Figma text box at `boxTop` with a line-height of `lineHeight` em.
// "normal" is 1.2 for this font, so 1.0 ("leading-none") draws the glyphs 0.1 em above the box.
float glyphTop(float boxTop, const theme::hud::TextStyle& style, float lineHeight);

// Text with its Figma text box's top-left corner at `topLeft`, snapped to whole pixels so glyphs
// rasterized at this exact size stay sharp.
void drawText(const Assets& assets, const theme::hud::TextStyle& style, const char* text,
              Vector2 topLeft, Color color);

// Text vertically centered in `box`; `align` 0 = left edge + padding, 0.5 = centered,
// 1 = right edge - padding.
void drawTextInBox(const Assets& assets, const theme::hud::TextStyle& style, const char* text,
                   Rectangle box, float paddingX, float align);

// Word-wrapped paragraph inside `width`, lines `lineHeight` em apart (Figma line-height).
// Returns the height used. No allocations: wraps by scanning the string in place.
float drawParagraph(const Assets& assets, const theme::hud::TextStyle& style, const char* text,
                    Vector2 topLeft, float width, float lineHeight, Color color);

// Rounded rectangle with a corner radius in pixels (raylib takes a 0-1 "roundness").
void fillRounded(Rectangle rect, float radius, Color color);

// Figma groups clip their children to the rounded corners ("overflow: clip"). To draw a child
// that touches a corner (an accent strip, an end segment, a selected last row), fill the whole
// rounded parent in the child's color, but only inside the child's rectangle.
void fillClippedToRounded(Rectangle parent, float radius, Rectangle child, Color color);

float screenWidth();
float screenHeight();

// Copies `text` into `out` in upper case (ASCII), as Figma's "uppercase" text style does.
void toUpper(const char* text, char* out, std::size_t size);

} // namespace ui
} // namespace gridiron
