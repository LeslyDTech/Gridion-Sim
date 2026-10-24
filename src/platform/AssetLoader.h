#pragma once

#include "AssetDecode.h"
#include "Theme.h"
#include "core/PlayLoader.h"
#include "core/Player.h"

#include <atomic>
#include <cstddef>
#include <deque>
#include <filesystem>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace gridiron {

class Assets;
struct AppContext;

// One thing to load. Paths are relative to the executable's folder, with forward slashes, exactly
// as the loading screen shows them ("assets/sprites/players.png").
struct ManifestEntry {
    enum class Kind { Texture, Font, Plays, Defense };
    enum class TextureSlot { PlayerAtlas, Ball, Grass };
    enum class FontRole { Hud, Field, Debug };

    Kind kind;
    std::string path;
    // Texture
    TextureSlot slot = TextureSlot::Ball;
    // Font
    FontRole role = FontRole::Hud;
    theme::hud::Weight weight = theme::hud::Weight::Bold;
    int pixelSize = 0;
};

// Everything the game loads from disk except the loading screen's own fonts.
std::vector<ManifestEntry> defaultManifest();

// Streams the manifest in without freezing the window.
//
// A worker thread does every slow, CPU-only part: reading files, decoding PNGs, rasterizing font
// glyphs and packing their atlases, and parsing play JSON. Results go into a queue. The main
// thread then calls pump() once per frame, which takes a few finished items off the queue and
// creates their GPU textures (LoadTextureFromImage). That part MUST stay on the main thread: the
// OpenGL context belongs to the thread that created the window, and GL calls from any other
// thread fail or crash. Uploading only a few per frame keeps the loading screen animating.
//
// Progress counts items actually installed into Assets, not elapsed time, so the bar is real.
class AssetLoader {
public:
    AssetLoader(std::filesystem::path root, std::vector<ManifestEntry> manifest);
    ~AssetLoader(); // stops and joins the worker; frees anything decoded but not installed
    AssetLoader(const AssetLoader&) = delete;
    AssetLoader& operator=(const AssetLoader&) = delete;

    void start();

    // Main thread only: installs up to `budget` decoded items (GPU upload + hand-off).
    void pump(Assets& assets, AppContext& context, int budget);

    std::size_t total() const { return m_manifest.size(); }
    std::size_t installed() const { return m_installed; }
    float progress() const;
    bool finished() const { return m_installed == m_manifest.size(); }

    // The file the worker is reading right now (or the last one, once done).
    std::string currentPath() const;

    // "assets/... (missing file)" for every entry that failed.
    const std::vector<std::string>& errors() const { return m_errors; }
    // True if the plays or the defense failed to load: the game can't start without them.
    bool fatal() const { return m_fatal; }

private:
    struct Decoded {
        std::size_t entry = 0;
        bool ok = false;
        std::string error;
        Image image{};
        DecodedFont font;
        Playbook playbook;
    };

    void workerMain();
    Decoded decode(std::size_t index) const;
    void install(Decoded& item, Assets& assets, AppContext& context);

    std::filesystem::path m_root;
    std::vector<ManifestEntry> m_manifest;

    std::thread m_worker;
    std::atomic<bool> m_stop{false};
    mutable std::mutex m_mutex; // guards m_ready and m_current
    std::deque<Decoded> m_ready;
    std::string m_current;

    // Main thread only.
    std::size_t m_installed = 0;
    std::vector<std::string> m_errors;
    bool m_fatal = false;
};

} // namespace gridiron
