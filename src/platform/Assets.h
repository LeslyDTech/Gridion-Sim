#pragma once

#include "Theme.h"
#include "core/Player.h"

#include <raylib.h>

#include <array>
#include <cstddef>
#include <filesystem>

namespace gridiron {

// RAII owner of every GPU resource: fonts, sprites, tiles and the generated textures (vignettes,
// shadow). Must be constructed after the window exists (raylib needs an OpenGL context) and
// destroyed before it closes; App's member order guarantees both.
//
// Assets is the cache; it doesn't read files itself. The generated textures are made in the
// constructor, the loading screen's own fonts come from loadBootFonts(), and everything else is
// installed by the AssetLoader as the loading screen streams it in.
//
// Everything is optional: a missing texture returns nullptr (the renderer draws shapes instead) and
// a missing font falls back to raylib's built-in font, so the game still runs if a file is lost.
class Assets {
public:
    static constexpr int kSkinTones = 5; // sprite variants per jersey color
    // Debug overlay font size: JetBrains Mono's advance is exactly 10 px at this size.
    static constexpr int kDebugFontSize = 22;
    // Field paint (yard numbers, end zones) and jersey numbers: Bebas Neue at this size.
    static constexpr int kFieldFontSize = 96;

    Assets(); // generates the procedural textures
    ~Assets();
    Assets(const Assets&) = delete;
    Assets& operator=(const Assets&) = delete;

    // Loads, synchronously, just the fonts the loading screen itself needs (it can't show a
    // progress bar for its own text). Everything else goes through the AssetLoader.
    void loadBootFonts(const std::filesystem::path& assetDirectory);

    // --- Installed by the AssetLoader (Assets takes ownership) ---
    void setFieldFont(Font font);
    void setDebugFont(Font font);
    void addHudFont(theme::hud::Weight weight, int pixelSize, Font font);
    void setPlayerSprite(Team team, int variant, Texture2D texture);
    void setBallSprite(Texture2D texture);
    void setGrassTiles(Texture2D texture);

    // --- Access ---
    // Field paint / jersey number font (Bebas Neue), or raylib's built-in font if missing.
    const Font& font() const { return m_hasFieldFont ? m_fieldFont : m_defaultFont; }
    // HUD and menu font (Barlow Condensed) at the loaded size nearest `pixelSize`.
    const Font& hudFont(theme::hud::Weight weight, float pixelSize) const;
    // Monospace font for the F1 debug overlay (JetBrains Mono at kDebugFontSize).
    const Font& debugFont() const { return m_hasDebugFont ? m_debugFont : m_defaultFont; }

    // Transparent in the middle, dark at the edges; stretched over the whole screen.
    const Texture2D& vignette() const { return m_vignette; }
    // The menus' "Soft field vignette" (Figma), stretched over the whole screen.
    const Texture2D& softFieldVignette() const { return m_softFieldVignette; }
    // Soft round shadow: dark in the middle, fading to fully transparent at the edge.
    const Texture2D& softShadow() const { return m_softShadow; }

    // Top-down player sprite, facing +x. `variant` picks one of kSkinTones skin tones.
    const Texture2D* playerSprite(Team team, int variant) const;
    const Texture2D* ballSprite() const { return valid(m_ball); }
    // Tilesheet with 64x64 tiles; tile (0, 0) is plain striped grass.
    const Texture2D* grassTiles() const { return valid(m_grass); }

private:
    static const Texture2D* valid(const Texture2D& texture) {
        return texture.id != 0 ? &texture : nullptr;
    }

    Font m_defaultFont{};
    Font m_fieldFont{};
    bool m_hasFieldFont = false;
    Font m_debugFont{};
    bool m_hasDebugFont = false;
    Texture2D m_vignette{};
    Texture2D m_softShadow{};
    Texture2D m_softFieldVignette{};

    struct HudFont {
        theme::hud::Weight weight;
        int pixelSize;
        Font font;
    };
    static constexpr std::size_t kMaxHudFonts = 16;
    std::array<HudFont, kMaxHudFonts> m_hudFonts{};
    std::size_t m_hudFontCount = 0;

    std::array<Texture2D, kSkinTones> m_homeSprites{};
    std::array<Texture2D, kSkinTones> m_awaySprites{};
    Texture2D m_ball{};
    Texture2D m_grass{};
};

} // namespace gridiron
