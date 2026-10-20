#include "Assets.h"

#include "Theme.h"

#include <string>

namespace gridiron {
namespace {

// Glyphs are rasterized once at this size; mipmaps keep them smooth when drawn smaller.
constexpr int kFontAtlasSize = 96;
constexpr int kVignetteSize = 256;

// Loads a texture, or returns an empty one (id == 0) if the file is missing.
//
// Sprites are drawn rotated and scaled down (a 31 px sprite becomes ~20 px on screen), so they
// use mipmaps + trilinear filtering. Point filtering keeps pixel art crisp only at integer scales
// and would shimmer here.
Texture2D loadTexture(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        TraceLog(LOG_WARNING, "Texture not found: %s (falling back to shapes)",
                 path.string().c_str());
        return Texture2D{};
    }
    Texture2D texture = LoadTexture(path.string().c_str());
    if (texture.id != 0) {
        GenTextureMipmaps(&texture);
        SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
    }
    return texture;
}

void unload(Texture2D& texture) {
    if (texture.id != 0) {
        UnloadTexture(texture);
        texture = Texture2D{};
    }
}

} // namespace

Assets::Assets(const std::filesystem::path& assetDirectory) : m_defaultFont(GetFontDefault()) {
    const std::filesystem::path fontPath = assetDirectory / "fonts" / "BebasNeue-Regular.ttf";
    if (std::filesystem::exists(fontPath)) {
        m_font = LoadFontEx(fontPath.string().c_str(), kFontAtlasSize, nullptr, 0);
        m_hasCustomFont = IsFontValid(m_font);
        if (m_hasCustomFont) {
            GenTextureMipmaps(&m_font.texture);
            SetTextureFilter(m_font.texture, TEXTURE_FILTER_TRILINEAR);
        }
    }
    if (!m_hasCustomFont) {
        TraceLog(LOG_WARNING, "Font not found at %s, using the default font",
                 fontPath.string().c_str());
    }

    // The vignette is generated rather than shipped as a file: a radial gradient from fully
    // transparent in the center to the theme's dark color at the corners.
    Image image = GenImageGradientRadial(kVignetteSize, kVignetteSize, 0.35f,
                                         Fade(theme::kUiDark, 0.0f), theme::kUiDark);
    m_vignette = LoadTextureFromImage(image);
    UnloadImage(image);
    SetTextureFilter(m_vignette, TEXTURE_FILTER_BILINEAR);

    // Kenney Sports Pack (CC0): blue jerseys for the offense, orange for the defense.
    const std::filesystem::path players = assetDirectory / "sprites" / "players";
    for (int i = 0; i < kSkinTones; ++i) {
        const std::string suffix = "_" + std::to_string(i + 1) + ".png";
        m_homeSprites[static_cast<std::size_t>(i)] = loadTexture(players / ("blue" + suffix));
        m_awaySprites[static_cast<std::size_t>(i)] = loadTexture(players / ("orange" + suffix));
    }
    m_ball = loadTexture(assetDirectory / "sprites" / "ball_football.png");
    m_grass = loadTexture(assetDirectory / "tiles" / "field_grass.png");
}

Assets::~Assets() {
    for (Texture2D& texture : m_homeSprites) {
        unload(texture);
    }
    for (Texture2D& texture : m_awaySprites) {
        unload(texture);
    }
    unload(m_ball);
    unload(m_grass);
    UnloadTexture(m_vignette);
    if (m_hasCustomFont) {
        UnloadFont(m_font); // the default font belongs to raylib
    }
}

const Texture2D* Assets::playerSprite(Team team, int variant) const {
    const auto& sprites = team == Team::Offense ? m_homeSprites : m_awaySprites;
    const auto index = static_cast<std::size_t>(variant % kSkinTones);
    return valid(sprites[index]);
}

} // namespace gridiron
