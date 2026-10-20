#include "AssetDecode.h"

#include <array>
#include <cmath>
#include <cstddef>
#include <vector>

namespace gridiron {
namespace {

// Glyph padding inside the atlas, as raylib's own font loader uses.
constexpr int kGlyphPadding = 4;

// ASCII, plus the middle dot and up/down arrows used by the HUD and menu instruction bars.
constexpr std::array<int, 98> makeCodepoints() {
    std::array<int, 98> codepoints{};
    for (int i = 0; i < 95; ++i) {
        codepoints[static_cast<std::size_t>(i)] = 32 + i;
    }
    codepoints[95] = 0x00B7; // middle dot
    codepoints[96] = 0x2191; // up arrow
    codepoints[97] = 0x2193; // down arrow
    return codepoints;
}
constexpr std::array<int, 98> kCodepoints = makeCodepoints();

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

} // namespace

bool decodeImage(const std::filesystem::path& path, Image& out, std::string& error) {
    if (!std::filesystem::exists(path)) {
        error = "missing file";
        return false;
    }
    out = LoadImage(path.string().c_str());
    if (out.data == nullptr) {
        error = "could not decode image";
        return false;
    }
    bleedColorIntoTransparentPixels(out);
    return true;
}

bool decodeFont(const std::filesystem::path& path, int pixelSize, DecodedFont& out,
                std::string& error) {
    if (!std::filesystem::exists(path)) {
        error = "missing file";
        return false;
    }
    int dataSize = 0;
    unsigned char* data = LoadFileData(path.string().c_str(), &dataSize);
    if (data == nullptr) {
        error = "could not read file";
        return false;
    }
    const int count = static_cast<int>(kCodepoints.size());
    // LoadFontData only reads its codepoint list, but its C signature isn't const.
    int* codepoints = const_cast<int*>(kCodepoints.data());
    GlyphInfo* glyphs = LoadFontData(data, dataSize, pixelSize, codepoints, count, FONT_DEFAULT);
    if (glyphs == nullptr) {
        UnloadFileData(data);
        error = "could not rasterize font";
        return false;
    }

    // raylib stores glyph advances as integers and truncates them, losing ~0.5 px per character
    // at small sizes, so a 45-character HUD line came out ~20 px narrower than the Figma design.
    // Re-derive each advance from an 8x rasterization and *round* it: the per-glyph error becomes
    // +/-0.5 px with zero mean, so line widths match the design. Glyph images are unchanged.
    constexpr int kPrecision = 8;
    GlyphInfo* precise =
        LoadFontData(data, dataSize, pixelSize * kPrecision, codepoints, count, FONT_DEFAULT);
    if (precise != nullptr) {
        for (int i = 0; i < count; ++i) {
            glyphs[i].advanceX =
                static_cast<int>(std::lround(static_cast<float>(precise[i].advanceX) / kPrecision));
        }
        UnloadFontData(precise, count);
    }
    UnloadFileData(data);

    out.glyphs = glyphs;
    out.glyphCount = count;
    out.baseSize = pixelSize;
    out.atlas = GenImageFontAtlas(glyphs, &out.recs, count, pixelSize, kGlyphPadding, 0);
    return true;
}

Texture2D uploadTexture(Image& image) {
    Texture2D texture = LoadTextureFromImage(image);
    UnloadImage(image);
    image = Image{};
    if (texture.id != 0) {
        // Sprites are drawn rotated and scaled down, so mipmaps + trilinear. Wrapping is clamped:
        // with the default (repeat), filtering at a sprite's edge samples the opposite edge.
        GenTextureMipmaps(&texture);
        SetTextureFilter(texture, TEXTURE_FILTER_TRILINEAR);
        SetTextureWrap(texture, TEXTURE_WRAP_CLAMP);
    }
    return texture;
}

Font uploadFont(DecodedFont& decoded) {
    Font font{};
    font.baseSize = decoded.baseSize;
    font.glyphCount = decoded.glyphCount;
    font.glyphPadding = kGlyphPadding;
    font.glyphs = decoded.glyphs;
    font.recs = decoded.recs;
    font.texture = LoadTextureFromImage(decoded.atlas);
    UnloadImage(decoded.atlas);
    decoded = DecodedFont{}; // ownership moved into `font`

    // Text drawn at exactly this size on whole pixels stays crisp with plain bilinear sampling.
    // Large sizes are also drawn scaled (the yards popup animates in from 60%), so they get mips.
    constexpr int kScaledFontMinSize = 64;
    if (font.baseSize >= kScaledFontMinSize) {
        GenTextureMipmaps(&font.texture);
        SetTextureFilter(font.texture, TEXTURE_FILTER_TRILINEAR);
    } else {
        SetTextureFilter(font.texture, TEXTURE_FILTER_BILINEAR);
    }
    return font;
}

void freeDecoded(Image& image) {
    if (image.data != nullptr) {
        UnloadImage(image);
    }
    image = Image{};
}

void freeDecoded(DecodedFont& font) {
    if (font.glyphs != nullptr) {
        UnloadFontData(font.glyphs, font.glyphCount);
    }
    if (font.recs != nullptr) {
        MemFree(font.recs);
    }
    if (font.atlas.data != nullptr) {
        UnloadImage(font.atlas);
    }
    font = DecodedFont{};
}

} // namespace gridiron
