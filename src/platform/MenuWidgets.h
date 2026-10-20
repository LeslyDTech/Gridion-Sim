#pragma once

#include <raylib.h>

namespace gridiron {

class Assets;
struct MenuCommands;

// Building blocks shared by every menu scene, drawn to the Figma frames on "Page 2" (all sizes and
// colors come from theme::menu).
namespace menu {

// Where a vertical list of options sits on screen.
struct ListLayout {
    float x;
    float y;
    float width;
};

// "Football field" placeholder (Loading only, before the real field can be drawn): flat green with
// white vertical lines every 128 px.
void drawFieldPlaceholder();

// "Field darkening overlay" + "Soft field vignette", over whatever is behind the menu.
void drawBackdrop(const Assets& assets);

// "GRIDIRON SIM brand": the 96 px logo centered (Loading, Title) or the 48 px logo at the left
// (Main Menu), with the yellow underline when `underlineWidth` > 0.
void drawBrandCentered(const Assets& assets, float top, float underlineTop, float underlineWidth);
void drawBrandLeft(const Assets& assets, float left, float top, float underlineTop,
                   float underlineWidth);

// A column of options: the selected row gets the blue fill and white left border.
// `values` (optional, may be null) are drawn right-aligned in each row (settings).
void drawOptions(const Assets& assets, ListLayout layout, const char* const* labels,
                 const char* const* values, int count, int selected);

// Index of the option under the mouse, or -1.
int optionAt(ListLayout layout, int count, float mouseX, float mouseY);

// "Selected option information": accent strip, title and word-wrapped description.
void drawInfoPanel(const Assets& assets, const char* title, const char* description);

// "Navigation controls bar" at the bottom of the screen.
void drawNavigationBar(const Assets& assets, const char* text);

// Keyboard / gamepad / mouse selection over a list of `count` options.
class Selection {
public:
    explicit Selection(int count, int initial = 0) : m_count(count), m_selected(initial) {}

    // Moves the selection; returns the option confirmed this frame (Enter / A / click), or -1.
    int update(const MenuCommands& input, ListLayout layout);

    int selected() const { return m_selected; }

private:
    int m_count;
    int m_selected;
};

} // namespace menu
} // namespace gridiron
