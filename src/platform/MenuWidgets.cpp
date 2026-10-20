#include "MenuWidgets.h"

#include "Assets.h"
#include "InputDevice.h"
#include "Theme.h"
#include "UiDraw.h"

#include <cmath>

namespace gridiron::menu {

using namespace theme::menu;

void drawFieldPlaceholder() {
    const float width = ui::screenWidth();
    const float height = ui::screenHeight();
    DrawRectangleRec(Rectangle{0.0f, 0.0f, width, height}, kFootballField);
    for (float x = kVerticalYardLineSpacing; x < width; x += kVerticalYardLineSpacing) {
        DrawRectangleRec(Rectangle{x, 0.0f, 1.0f, height}, kVerticalYardLine);
    }
}

void drawBackdrop(const Assets& assets) {
    const Rectangle screen{0.0f, 0.0f, ui::screenWidth(), ui::screenHeight()};
    DrawRectangleRec(screen, kFieldDarkeningOverlay);
    const Texture2D& vignette = assets.softFieldVignette();
    DrawTexturePro(vignette,
                   Rectangle{0.0f, 0.0f, static_cast<float>(vignette.width),
                             static_cast<float>(vignette.height)},
                   screen, Vector2{}, 0.0f, WHITE); // WHITE = the texture's own colors
}

void drawBrandCentered(const Assets& assets, float top, float underlineTop, float underlineWidth) {
    const TextStyle& style = kGridironSimLogoLarge;
    const Vector2 size = ui::measure(assets, style, kGridironSimLogoText);
    const float centerX = ui::screenWidth() / 2.0f;
    ui::drawText(assets, style, kGridironSimLogoText,
                 Vector2{centerX - size.x / 2.0f, ui::glyphTop(top, style, kLineHeightNone)},
                 style.color);
    if (underlineWidth > 0.0f) {
        DrawRectangleRec(Rectangle{std::round(centerX - underlineWidth / 2.0f), underlineTop,
                                   underlineWidth, kBrandUnderlineHeight},
                         kBrandUnderline);
    }
}

void drawBrandLeft(const Assets& assets, float left, float top, float underlineTop,
                   float underlineWidth) {
    const TextStyle& style = kGridironSimLogoSmall;
    ui::drawText(assets, style, kGridironSimLogoText,
                 Vector2{left, ui::glyphTop(top, style, kLineHeightNone)}, style.color);
    if (underlineWidth > 0.0f) {
        DrawRectangleRec(Rectangle{left, underlineTop, underlineWidth, kBrandUnderlineHeight},
                         kBrandUnderline);
    }
}

namespace {

Rectangle rowRect(ListLayout layout, int index) {
    return {layout.x, layout.y + static_cast<float>(index) * (kMenuOptionHeight + kMenuOptionGap),
            layout.width, kMenuOptionHeight};
}

} // namespace

void drawOptions(const Assets& assets, ListLayout layout, const char* const* labels,
                 const char* const* values, int count, int selected) {
    for (int i = 0; i < count; ++i) {
        const Rectangle row = rowRect(layout, i);
        const bool isSelected = i == selected;
        if (isSelected) {
            DrawRectangleRec(row, kSelectedMenuOptionFill);
            DrawRectangleRec(Rectangle{row.x, row.y, kSelectedMenuOptionBorderWidth, row.height},
                             kSelectedMenuOptionBorder);
        }
        TextStyle style = kOptionLabel;
        style.color = isSelected ? kOptionLabelSelected : kOptionLabel.color;
        ui::drawTextInBox(assets, style, labels[i], row, kMenuOptionPaddingX, 0.0f);
        if (values != nullptr && values[i] != nullptr) {
            ui::drawTextInBox(assets, style, values[i], row, kMenuOptionPaddingX, 1.0f);
        }
    }
}

int optionAt(ListLayout layout, int count, float mouseX, float mouseY) {
    for (int i = 0; i < count; ++i) {
        if (CheckCollisionPointRec(Vector2{mouseX, mouseY}, rowRect(layout, i))) {
            return i;
        }
    }
    return -1;
}

void drawInfoPanel(const Assets& assets, const char* title, const char* description) {
    const Rectangle panel{ui::screenWidth() - kSelectedOptionInformationRight -
                              kSelectedOptionInformationWidth,
                          kSelectedOptionInformationTop, kSelectedOptionInformationWidth,
                          kSelectedOptionInformationHeight};
    ui::fillRounded(panel, kSelectedOptionInformationCornerRadius,
                    kSelectedOptionInformationBackground);
    ui::fillClippedToRounded(panel, kSelectedOptionInformationCornerRadius,
                             Rectangle{panel.x, panel.y, kInformationAccentWidth, panel.height},
                             kInformationAccent);

    const float textLeft = panel.x + kSelectedOptionInformationPadding;
    const float textWidth = panel.width - 2.0f * kSelectedOptionInformationPadding;
    float top = panel.y + kSelectedOptionInformationPadding;
    top += ui::drawParagraph(assets, kSelectedOptionTitle, title, Vector2{textLeft, top}, textWidth,
                             kSelectedOptionTitleLineHeight, kSelectedOptionTitle.color);
    top += kSelectedOptionInformationGap;
    ui::drawParagraph(assets, kSelectedOptionDescription, description, Vector2{textLeft, top},
                      textWidth, kSelectedOptionDescriptionLineHeight,
                      kSelectedOptionDescription.color);
}

void drawNavigationBar(const Assets& assets, const char* text) {
    const Rectangle bar{0.0f, ui::screenHeight() - kNavigationControlsBarHeight, ui::screenWidth(),
                        kNavigationControlsBarHeight};
    DrawRectangleRec(bar, kNavigationControlsBarBackground);
    ui::drawTextInBox(assets, kNavigationInstructions, text, bar, 0.0f, 0.5f);
}

int Selection::update(const MenuCommands& input, ListLayout layout) {
    if (m_count <= 0) {
        return -1;
    }
    if (input.up) {
        m_selected = (m_selected + m_count - 1) % m_count;
    }
    if (input.down) {
        m_selected = (m_selected + 1) % m_count;
    }
    const int hovered = optionAt(layout, m_count, input.mouseX, input.mouseY);
    if (hovered >= 0 && (input.mouseMoved || input.mouseClicked)) {
        m_selected = hovered;
    }
    if (input.confirm || (input.mouseClicked && hovered >= 0)) {
        return m_selected;
    }
    return -1;
}

} // namespace gridiron::menu
