#include "Assets.h"

#include "Theme.h"

#include <algorithm>
#include <cmath>
#include <string>
#include <utility>
#include <vector>

namespace gridiron {
namespace {

// Glyphs are rasterized once at this size; mipmaps keep them smooth when drawn smaller.
constexpr int kFontAtlasSize = 96;
constexpr int kVignetteSize = 256;
constexpr int kShadowSize = 64;

// Loads a texture, or returns an empty one (id == 0) if the file is missing.
//
// Sprites are drawn rotated and scaled down (a 31 px sprite becomes ~20 px on screen), so they
// use mipmaps + trilinear filtering. Point filtering keeps pixel art crisp only at integer scales
// and would shimmer here.
// Fully transparent pixels in PNGs are usually black. Filtering (bilinear, mipmaps) blends them
// into visible edge pixels, which leaves a dark fringe around rotated or scaled sprites. This
// copies each transparent pixel's color from its opaque neighbours, growing outward a few pixels,
// while keeping alpha at 0 so nothing new becomes visible.
void bleedColorIntoTransparentPixels(Image& image) {
    ImageFormat(&image, PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    const int width = image.width;
    const int height = image.height;
    auto* pixels = static_cast<Color*>(image.data);
    std::vector<unsigned char> filled(static_cast<std::size_t>(width * height));
    for (int i = 0; i < width * height; ++i) {
        filled[static_cast<std::size_t>(i)] = pixels[i].a > 0 ? 1 : 0;
    }

    constexpr int kPasses = 4; // enough for the deepest mip levels the sprites are drawn at
    std::vector<unsigned char> next = filled;
    for (int pass = 0; pass < kPasses; ++pass) {
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const int index = y * width + x;
                if (filled[static_cast<std::size_t>(index)] != 0) {
                    continue;
                }
                int r = 0, g = 0, b = 0, count = 0;
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        const int nx = x + dx;
                        const int ny = y + dy;
                        if (nx < 0 || ny < 0 || nx >= width || ny >= height) {
                            continue;
                        }
                        const int n = ny * width + nx;
                        if (filled[static_cast<std::size_t>(n)] != 0) {
                            r += pixels[n].r;
                            g += pixels[n].g;
                            b += pixels[n].b;
                            ++count;
                        }
                    }
                }
                if (count > 0) {
                    pixels[index] = Color{static_cast<unsigned char>(r / count),
                                          static_cast<unsigned char>(g / count),
                                          static_cast<unsigned char>(b / count), 0};
                    next[static_cast<std::size_t>(index)] = 1;
                }
            }
        }
        filled = next;
    }
}

// Loads a texture, or returns an empty one (id == 0) if the file is missing.
//
// Sprites are drawn rotated and scaled down (a 31 px sprite becomes ~20 px on screen), so they
// use mipmaps + trilinear filtering. Point filtering keeps pixel art crisp only at integer scales
// and would shimmer here. Wrapping is clamped: with the default (repeat), filtering at a sprite's
// edge samples the opposite edge, which drew thin lines along rotated sprites.
Texture2D loadTexture(const std::filesystem::path& path) {
    if (!std::filesystem::exists(path)) {
        TraceLog(LOG_WARNING, "Texture not found: %s (falling back to shapes)",
                 path.string().c_str());
        return Texture2D{};
    }
    Image image = LoadImage(path.string().c_str());
    if (image.data == nullptr) {
        return Texture2D{};
    }
    bleedColorIntoTransparentPixels(image);
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    if (texture.id != 0) {
        GenTextureMipmaps(&texture);
        SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
        SetTextureWrap(texture, TEXTURE_WRAP_CLAMP);
    }
    return texture;
}

// Rasterizes a TTF at one exact pixel size: ASCII plus U+00B7 (the middle dot used in the HUD).
Font loadFontAtSize(const std::filesystem::path& path, int pixelSize) {
    // ASCII, plus the middle dot and up/down arrows used by the HUD and menu instruction bars.
    std::array<int, 98> codepoints{};
    for (int i = 0; i < 95; ++i) {
        codepoints[static_cast<std::size_t>(i)] = 32 + i;
    }
    codepoints[95] = 0x00B7; // middle dot
    codepoints[96] = 0x2191; // up arrow
    codepoints[97] = 0x2193; // down arrow
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
        GlyphInfo* precise = LoadFontData(data, dataSize, pixelSize * kPrecision, codepoints.data(),
                                          count, FONT_DEFAULT);
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

    // Player shadow, also generated: solid for the inner 30% of the radius, then a smooth falloff
    // to transparent, which gives the soft edge.
    Image shadow = GenImageGradientRadial(kShadowSize, kShadowSize, 0.3f, theme::kUiDark,
                                          Fade(theme::kUiDark, 0.0f));
    m_softShadow = LoadTextureFromImage(shadow);
    UnloadImage(shadow);
    SetTextureFilter(m_softShadow, TEXTURE_FILTER_BILINEAR);

    m_softFieldVignette = makeSoftFieldVignette();

    // Kenney Sports Pack (CC0): blue jerseys for the offense, orange for the defense.
    const std::filesystem::path players = assetDirectory / "sprites" / "players";
    for (int i = 0; i < kSkinTones; ++i) {
        const std::string suffix = "_" + std::to_string(i + 1) + ".png";
        m_homeSprites[static_cast<std::size_t>(i)] = loadTexture(players / ("blue" + suffix));
        m_awaySprites[static_cast<std::size_t>(i)] = loadTexture(players / ("orange" + suffix));
    }
    m_ball = loadTexture(assetDirectory / "sprites" / "ball_football.png");

    // HUD and menu text sizes: Figma font size x1.2 (see ui::kFigmaToRaylibFontScale).
    //   Bold:     14, 18, 20, 24, 28, 40, 48, 64, 96 px
    //   SemiBold: 12, 14, 16, 18, 24 px
    using theme::hud::Weight;
    const std::filesystem::path fonts = assetDirectory / "fonts";
    constexpr std::array<std::pair<Weight, int>, kHudFontCount> kSizes = {{
        {Weight::Bold, 17},
        {Weight::Bold, 22},
        {Weight::Bold, 24},
        {Weight::Bold, 29},
        {Weight::Bold, 34},
        {Weight::Bold, 48},
        {Weight::Bold, 58},
        {Weight::Bold, 77},
        {Weight::Bold, 115},
        {Weight::SemiBold, 14},
        {Weight::SemiBold, 17},
        {Weight::SemiBold, 19},
        {Weight::SemiBold, 22},
        {Weight::SemiBold, 29},
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

    const std::filesystem::path mono = fonts / "JetBrainsMono-Regular.ttf";
    if (std::filesystem::exists(mono)) {
        m_debugFont = loadFontAtSize(mono, kDebugFontSize);
        m_hasDebugFont = IsFontValid(m_debugFont);
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
    for (HudFont& hud : m_hudFonts) {
        if (IsFontValid(hud.font)) {
            UnloadFont(hud.font);
        }
    }
    if (m_hasDebugFont) {
        UnloadFont(m_debugFont);
    }
    UnloadTexture(m_softShadow);
    UnloadTexture(m_softFieldVignette);
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
