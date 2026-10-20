#pragma once

#include <raylib.h>

#include <filesystem>

namespace gridiron {

// RAII owner of GPU resources loaded once at startup: the UI font and the vignette texture.
// Must be constructed after the window exists (raylib needs an OpenGL context) and destroyed
// before it closes. App's member order guarantees both.
class Assets {
public:
    explicit Assets(const std::filesystem::path& assetDirectory);
    ~Assets();
    Assets(const Assets&) = delete;
    Assets& operator=(const Assets&) = delete;

    // The broadcast TTF font, or raylib's built-in font if the file was missing.
    const Font& font() const { return m_hasCustomFont ? m_font : m_defaultFont; }
    bool hasCustomFont() const { return m_hasCustomFont; }

    // Transparent in the middle, dark at the edges; stretched over the whole screen.
    const Texture2D& vignette() const { return m_vignette; }

private:
    Font m_font{};
    Font m_defaultFont{};
    bool m_hasCustomFont = false;
    Texture2D m_vignette{};
};

} // namespace gridiron
