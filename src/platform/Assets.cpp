#include "Assets.h"

#include "AssetDecode.h"
#include "Theme.h"

#include <algorithm>
#include <cmath>
#include <rlgl.h>
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

// Skin-tone swap for the player atlas, which is exported with one skin tone (Skin/Medium).
// The art is flat vector fills, so skin is exactly one color, and its anti-aliased edges are mixes
// of that color and the outline. Each pixel is projected onto the outline -> Skin/Medium color
// line: pixels on that line (skin, or a skin/outline edge) are rebuilt at the same position on the
// outline -> new-skin line; every other color (jerseys, helmets, face masks) is left alone. The
// k > 0.25 ramp keeps near-outline colors, such as the Blaze's dark trim, untouched.
//
// Desktop OpenGL 3.3 wants "#version 330"; the browser build runs on WebGL 2 (OpenGL ES 3.0),
// which wants "#version 300 es" and a default float precision. The shader body is the same.
#if defined(__EMSCRIPTEN__)
#define GRIDIRON_GLSL_HEADER "#version 300 es\nprecision mediump float;\n"
#else
#define GRIDIRON_GLSL_HEADER "#version 330\n"
#endif
constexpr const char* kSkinSwapShader = GRIDIRON_GLSL_HEADER R"(
in vec2 fragTexCoord;
in vec4 fragColor;
uniform sampler2D texture0;
uniform vec4 colDiffuse;
uniform vec3 baseSkin;
uniform vec3 outlineColor;
uniform vec3 skin;
out vec4 finalColor;
void main() {
    vec4 texel = texture(texture0, fragTexCoord);
    vec3 axis = baseSkin - outlineColor;
    float k = clamp(dot(texel.rgb - outlineColor, axis) / dot(axis, axis), 0.0, 1.0);
    float offLine = distance(texel.rgb, outlineColor + axis * k);
    float weight = (1.0 - smoothstep(0.02, 0.06, offLine)) * smoothstep(0.25, 0.6, k);
    vec3 swapped = outlineColor + (skin - outlineColor) * k;
    finalColor = vec4(mix(texel.rgb, swapped, weight), texel.a) * colDiffuse * fragColor;
}
)";

Vector3 toVector(Color color) {
    return Vector3{color.r / 255.0f, color.g / 255.0f, color.b / 255.0f};
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

    // Shaders compile in well under a millisecond, so this one is built here, not streamed in.
    // nullptr: raylib's default vertex shader.
    m_skinShader = LoadShaderFromMemory(nullptr, kSkinSwapShader);
    if (IsShaderValid(m_skinShader) && m_skinShader.id != rlGetShaderIdDefault()) {
        const Vector3 base = toVector(theme::sprite::kSkinMedium);
        const Vector3 outline = toVector(theme::sprite::kOutline);
        SetShaderValue(m_skinShader, GetShaderLocation(m_skinShader, "baseSkin"), &base,
                       SHADER_UNIFORM_VEC3);
        SetShaderValue(m_skinShader, GetShaderLocation(m_skinShader, "outlineColor"), &outline,
                       SHADER_UNIFORM_VEC3);
        m_skinLocation = GetShaderLocation(m_skinShader, "skin");
    } else {
        m_skinShader = Shader{}; // compile failed: raylib handed back its default shader
    }
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
    unload(m_playerAtlas);
    if (m_skinShader.id != 0) {
        UnloadShader(m_skinShader);
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

void Assets::setPlayerAtlas(Texture2D texture) { replace(m_playerAtlas, texture); }

void Assets::setSkinTone(int tone) const {
    if (m_skinShader.id == 0) {
        return;
    }
    const auto index = static_cast<std::size_t>(tone % kSkinTones);
    const Vector3 skin = toVector(theme::sprite::kSkinTones[index]);
    SetShaderValue(m_skinShader, m_skinLocation, &skin, SHADER_UNIFORM_VEC3);
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

} // namespace gridiron
