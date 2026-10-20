#include "Assets.h"

#include "Theme.h"

#include <cmath>
#include <string>
#include <utility>

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

// Rasterizes a TTF at one exact pixel size: ASCII plus U+00B7 (the middle dot used in the HUD).
Font loadFontAtSize(const std::filesystem::path& path, int pixelSize) {
    std::array<int, 96> codepoints{};
    for (int i = 0; i < 95; ++i) {
        codepoints[static_cast<std::size_t>(i)] = 32 + i;
    }
    codepoints[95] = 0x00B7;
    const int count = static_cast<int>(codepoints.size());
    Font font = LoadFontEx(path.string().c_str(), pixelSize, codepoints.data(), count);
    if (!IsFontValid(font)) {
        return font;
    }

    // raylib stores glyph advances as integers and truncates them, losing ~0.5 px per character
    // at small sizes, so a 45-character HUD line came out ~20 px narrower than the Figma design.
    // Re-derive each advance from an 8x rasterization and *round* it: the per-glyph error becomes
    // +/-0.5 px with zero mean, so line widths match the design. Glyph images are unchanged.
    constexpr int kPrecision = 8;
    int dataSize = 0;
    unsigned char* data = LoadFileData(path.string().c_str(), &dataSize);
    if (data != nullptr) {
        GlyphInfo* precise = LoadFontData(data, dataSize, pixelSize * kPrecision,
                                          codepoints.data(), count, FONT_DEFAULT);
        if (precise != nullptr) {
            // Both arrays follow the order of `codepoints`.
            for (int i = 0; i < count && i < font.glyphCount; ++i) {
                font.glyphs[i].advanceX = static_cast<int>(
                    std::lround(static_cast<float>(precise[i].advanceX) / kPrecision));
            }
            UnloadFontData(precise, count);
        }
        UnloadFileData(data);
    }

    // HUD text is drawn at exactly this size on whole pixels, so plain bilinear sampling keeps it
    // crisp. Only the large popup font is drawn scaled (it animates in from 60%), so it alone
    // gets mipmaps.
    constexpr int kScaledFontMinSize = 64;
    if (pixelSize >= kScaledFontMinSize) {
        GenTextureMipmaps(&font.texture);
        SetTextureFilter(font.texture, TEXTURE_FILTER_TRILINEAR);
    } else {
        SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    }
    return font;
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

    // HUD text sizes: Figma font size x1.2 (see theme::hud). Bold: 14, 18, 20, 28, 64 px;
    // SemiBold: 14, 16 px.
    using theme::hud::Weight;
    const std::filesystem::path fonts = assetDirectory / "fonts";
    constexpr std::array<std::pair<Weight, int>, kHudFontCount> kSizes = {{
        {Weight::Bold, 17},
        {Weight::Bold, 22},
        {Weight::Bold, 24},
        {Weight::Bold, 34},
        {Weight::Bold, 77},
        {Weight::SemiBold, 17},
        {Weight::SemiBold, 19},
    }};
    for (std::size_t i = 0; i < kSizes.size(); ++i) {
        const auto [weight, size] = kSizes[i];
        const char* file =
            weight == Weight::Bold ? "BarlowCondensed-Bold.ttf" : "BarlowCondensed-SemiBold.ttf";
        Font font{};
        if (std::filesystem::exists(fonts / file)) {
            font = loadFontAtSize(fonts / file, size);
        }
        m_hudFonts[i] = HudFont{weight, size, font};
    }
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
    for (HudFont& hud : m_hudFonts) {
        if (IsFontValid(hud.font)) {
            UnloadFont(hud.font);
        }
    }
    UnloadTexture(m_vignette);
    if (m_hasCustomFont) {
        UnloadFont(m_font); // the default font belongs to raylib
    }
}

const Font& Assets::hudFont(theme::hud::Weight weight, float pixelSize) const {
    const HudFont* best = nullptr;
    float bestDifference = 0.0f;
    for (const HudFont& hud : m_hudFonts) {
        if (hud.weight != weight || !IsFontValid(hud.font)) {
            continue;
        }
        const float difference = std::abs(static_cast<float>(hud.pixelSize) - pixelSize);
        if (best == nullptr || difference < bestDifference) {
            best = &hud;
            bestDifference = difference;
        }
    }
    return best != nullptr ? best->font : font();
}

const Texture2D* Assets::playerSprite(Team team, int variant) const {
    const auto& sprites = team == Team::Offense ? m_homeSprites : m_awaySprites;
    const auto index = static_cast<std::size_t>(variant % kSkinTones);
    return valid(sprites[index]);
}

} // namespace gridiron
