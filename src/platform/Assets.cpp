#include "Assets.h"

#include "Theme.h"

namespace gridiron {
namespace {

// Glyphs are rasterized once at this size; mipmaps keep them smooth when drawn smaller.
constexpr int kFontAtlasSize = 96;
constexpr int kVignetteSize = 256;

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
}

Assets::~Assets() {
    UnloadTexture(m_vignette);
    if (m_hasCustomFont) {
        UnloadFont(m_font); // the default font belongs to raylib
    }
}

} // namespace gridiron
