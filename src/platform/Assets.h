#pragma once

#include "Theme.h"
#include "core/Player.h"

#include <raylib.h>

#include <array>
#include <filesystem>

namespace gridiron {

// RAII owner of GPU resources loaded once at startup: font, vignette, and the Kenney Sports Pack
// art (player sprites, ball sprite, grass tiles). Must be constructed after the window exists
// (raylib needs an OpenGL context) and destroyed before it closes; App's member order guarantees
// both.
//
// Every texture is optional: if a file is missing the accessor returns nullptr and the renderer
// falls back to the code-drawn look, so the game still runs from a bare checkout.
class Assets {
public:
    static constexpr int kSkinTones = 5; // sprite variants per jersey color

    explicit Assets(const std::filesystem::path& assetDirectory);
    ~Assets();
    Assets(const Assets&) = delete;
    Assets& operator=(const Assets&) = delete;

    // The broadcast TTF font, or raylib's built-in font if the file was missing.
    const Font& font() const { return m_hasCustomFont ? m_font : m_defaultFont; }

    // Broadcast HUD font (Barlow Condensed) rasterized at `pixelSize`, or the nearest size that
    // was preloaded (Hud draws at exactly those sizes). Falls back to font() if missing.
    const Font& hudFont(theme::hud::Weight weight, float pixelSize) const;

    // Transparent in the middle, dark at the edges; stretched over the whole screen.
    const Texture2D& vignette() const { return m_vignette; }

    // Top-down player sprite, facing +x. `variant` picks one of kSkinTones skin tones.
    const Texture2D* playerSprite(Team team, int variant) const;
    const Texture2D* ballSprite() const { return valid(m_ball); }
    // Tilesheet with 64x64 tiles; tile (0, 0) is plain striped grass.
    const Texture2D* grassTiles() const { return valid(m_grass); }

private:
    static const Texture2D* valid(const Texture2D& texture) {
        return texture.id != 0 ? &texture : nullptr;
    }

    Font m_font{};
    Font m_defaultFont{};
    bool m_hasCustomFont = false;
    Texture2D m_vignette{};

    struct HudFont {
        theme::hud::Weight weight;
        int pixelSize;
        Font font;
    };
    static constexpr std::size_t kHudFontCount = 7;
    std::array<HudFont, kHudFontCount> m_hudFonts{};
    std::array<Texture2D, kSkinTones> m_homeSprites{};
    std::array<Texture2D, kSkinTones> m_awaySprites{};
    Texture2D m_ball{};
    Texture2D m_grass{};
};

} // namespace gridiron
