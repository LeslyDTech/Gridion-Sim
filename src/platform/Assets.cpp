#include "Assets.h"

#include "AssetDecode.h"
#include "Theme.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

namespace gridiron {
namespace {

constexpr int kVignetteSize = 256;
constexpr int kShadowSize = 64;

// The menus' "Soft field vignette" from Figma: an elliptical radial gradient (radii 704 x 432 on a
// 1280 x 720 screen), clear out to 45% of the radius, then fading to 50% black at the edge and
// beyond. Computed per pixel at low resolution and stretched over the screen; it is a smooth
// gradient, so bilinear filtering hides the low resolution.
Texture2D makeSoftFieldVignette() {
    using namespace theme::menu;
    constexpr int kWidth = 320;
    constexpr int kHeight = 180;
    std::vector<Color> pixels(static_cast<std::size_t>(kWidth * kHeight));
    for (int y = 0; y < kHeight; ++y) {
        for (int x = 0; x < kWidth; ++x) {
            // Pixel center relative to the screen center, in units of the ellipse radii.
            const float u =
                ((static_cast<float>(x) + 0.5f) / kWidth - 0.5f) / kSoftFieldVignetteRadiusX;
            const float v =
                ((static_cast<float>(y) + 0.5f) / kHeight - 0.5f) / kSoftFieldVignetteRadiusY;
            const float t = std::sqrt(u * u + v * v);
            const float ramp = std::clamp((t - kSoftFieldVignetteClearUntil) /
                                              (1.0f - kSoftFieldVignetteClearUntil),
                                          0.0f, 1.0f);
            pixels[static_cast<std::size_t>(y * kWidth + x)] =
                Color{0, 0, 0, static_cast<unsigned char>(ramp * kSoftFieldVignetteEdgeAlpha)};
        }
    }
    // The Image only borrows `pixels`: LoadTextureFromImage copies them to the GPU.
    const Image image{pixels.data(), kWidth, kHeight, 1, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};
    Texture2D texture = LoadTextureFromImage(image);
    SetTextureFilter(texture, TEXTURE_FILTER_BILINEAR);
    return texture;
}

void unload(Texture2D& texture) {
    if (texture.id != 0) {
        UnloadTexture(texture);
        texture = Texture2D{};
    }
}

void replace(Texture2D& slot, Texture2D texture) {
    unload(slot);
    slot = texture;
}

} // namespace

Assets::Assets() : m_defaultFont(GetFontDefault()) {
    // The vignette is generated rather than shipped as a file: a radial gradient from fully
    // transparent in the center to the theme's dark color at the corners.
    Image image = GenImageGradientRadial(kVignetteSize, kVignetteSize, 0.35f,
                                         Fade(theme::kUiDark, 0.0f), theme::kUiDark);
    m_vignette = LoadTextureFromImage(image);
    UnloadImage(image);
    SetTextureFilter(m_vignette, TEXTURE_FILTER_BILINEAR);

    // Player shadow, also generated: solid for the inner 30% of the radius, then a smooth falloff
    // to transparent, which gives the soft edge.
    Image shadow = GenImageGradientRadial(kShadowSize, kShadowSize, 0.3f, theme::kUiDark,
                                          Fade(theme::kUiDark, 0.0f));
    m_softShadow = LoadTextureFromImage(shadow);
    UnloadImage(shadow);
    SetTextureFilter(m_softShadow, TEXTURE_FILTER_BILINEAR);

    m_softFieldVignette = makeSoftFieldVignette();
}

void Assets::loadBootFonts(const std::filesystem::path& assetDirectory) {
    using theme::hud::Weight;
    // The Loading frame uses: the 96 px logo (Bold), 14 px status and 18 px tip (SemiBold).
    struct Boot {
        Weight weight;
        const char* file;
        int pixelSize;
    };
    constexpr Boot kBootFonts[] = {
        {Weight::Bold, "BarlowCondensed-Bold.ttf", 115},
        {Weight::SemiBold, "BarlowCondensed-SemiBold.ttf", 17},
        {Weight::SemiBold, "BarlowCondensed-SemiBold.ttf", 22},
    };
    for (const Boot& boot : kBootFonts) {
        DecodedFont decoded;
        std::string error;
        if (decodeFont(assetDirectory / "fonts" / boot.file, boot.pixelSize, decoded, error)) {
            addHudFont(boot.weight, boot.pixelSize, uploadFont(decoded));
        } else {
            TraceLog(LOG_WARNING, "Boot font %s: %s", boot.file, error.c_str());
        }
    }
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
    for (std::size_t i = 0; i < m_hudFontCount; ++i) {
        UnloadFont(m_hudFonts[i].font);
    }
    if (m_hasDebugFont) {
        UnloadFont(m_debugFont);
    }
    if (m_hasFieldFont) {
        UnloadFont(m_fieldFont); // the default font belongs to raylib
    }
    UnloadTexture(m_softShadow);
    UnloadTexture(m_softFieldVignette);
    UnloadTexture(m_vignette);
}

void Assets::setFieldFont(Font font) {
    if (m_hasFieldFont) {
        UnloadFont(m_fieldFont);
    }
    m_fieldFont = font;
    m_hasFieldFont = IsFontValid(font);
}

void Assets::setDebugFont(Font font) {
    if (m_hasDebugFont) {
        UnloadFont(m_debugFont);
    }
    m_debugFont = font;
    m_hasDebugFont = IsFontValid(font);
}

void Assets::addHudFont(theme::hud::Weight weight, int pixelSize, Font font) {
    if (!IsFontValid(font)) {
        return;
    }
    for (std::size_t i = 0; i < m_hudFontCount; ++i) {
        if (m_hudFonts[i].weight == weight && m_hudFonts[i].pixelSize == pixelSize) {
            UnloadFont(m_hudFonts[i].font); // already have this size: keep the new one
            m_hudFonts[i].font = font;
            return;
        }
    }
    if (m_hudFontCount == m_hudFonts.size()) {
        UnloadFont(font); // table full; never happens with the shipped manifest
        return;
    }
    m_hudFonts[m_hudFontCount++] = HudFont{weight, pixelSize, font};
}

void Assets::setPlayerSprite(Team team, int variant, Texture2D texture) {
    auto& sprites = team == Team::Offense ? m_homeSprites : m_awaySprites;
    replace(sprites[static_cast<std::size_t>(variant % kSkinTones)], texture);
}

void Assets::setBallSprite(Texture2D texture) { replace(m_ball, texture); }
void Assets::setGrassTiles(Texture2D texture) { replace(m_grass, texture); }

const Font& Assets::hudFont(theme::hud::Weight weight, float pixelSize) const {
    const HudFont* best = nullptr;
    float bestDifference = 0.0f;
    for (std::size_t i = 0; i < m_hudFontCount; ++i) {
        const HudFont& hud = m_hudFonts[i];
        if (hud.weight != weight) {
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
