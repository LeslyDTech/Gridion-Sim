#pragma once

#include <raylib.h>

#include <filesystem>
#include <string>

namespace gridiron {

// Asset loading is split in two halves, because OpenGL calls must stay on the main thread (the
// GL context belongs to the thread that created the window):
//
//   decode*  -- CPU only: read the file, decode PNG pixels, rasterize font glyphs, pack the glyph
//               atlas. Safe on a worker thread.
//   upload*  -- creates the GPU texture (LoadTextureFromImage). Main thread only.
//
// The loading screen runs decode* on a worker and upload* a few per frame on the main thread.

// A font rasterized on the CPU, waiting for its atlas texture to be uploaded.
struct DecodedFont {
    GlyphInfo* glyphs = nullptr; // owns the glyph images (UnloadFontData)
    Rectangle* recs = nullptr;   // where each glyph sits in the atlas
    Image atlas{};
    int glyphCount = 0;
    int baseSize = 0;
};

// Loads a PNG and fixes its transparent edge pixels for filtering (see bleedColorIntoTransparent).
// Returns false and sets `error` if the file is missing or can't be decoded.
bool decodeImage(const std::filesystem::path& path, Image& out, std::string& error);

// Rasterizes a TTF at one exact pixel size (ASCII plus the HUD's middle dot and arrows), with glyph
// advances rounded from an 8x rasterization (raylib truncates them, which made text too narrow).
bool decodeFont(const std::filesystem::path& path, int pixelSize, DecodedFont& out,
                std::string& error);

// Main thread only. Take ownership of the decoded data.
Texture2D uploadTexture(Image& image);
Font uploadFont(DecodedFont& font);

// Frees decoded data that was never uploaded (e.g. the loader was cancelled).
void freeDecoded(Image& image);
void freeDecoded(DecodedFont& font);

} // namespace gridiron
