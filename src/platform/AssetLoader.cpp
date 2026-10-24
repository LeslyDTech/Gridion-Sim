#include "AssetLoader.h"

#include "AppContext.h"
#include "Assets.h"

#include <utility>

namespace gridiron {

std::vector<ManifestEntry> defaultManifest() {
    using Kind = ManifestEntry::Kind;
    using Slot = ManifestEntry::TextureSlot;
    using Role = ManifestEntry::FontRole;
    using theme::hud::Weight;
    std::vector<ManifestEntry> manifest;

    // Every player sprite, both teams, in one atlas (Figma "PlayerSprite"; see theme::sprite).
    manifest.push_back({Kind::Texture, "assets/sprites/players.png", Slot::PlayerAtlas});
    // Kenney Sports Pack (CC0).
    manifest.push_back({Kind::Texture, "assets/sprites/ball_football.png", Slot::Ball});
    manifest.push_back({Kind::Texture, "assets/tiles/field_grass.png", Slot::Grass});

    // HUD and menu fonts at every Figma size x1.2 (see ui::kFigmaToRaylibFontScale). The loading
    // screen's own sizes (Bold 115, SemiBold 17 and 22) are loaded before this, at boot.
    const char* bold = "assets/fonts/BarlowCondensed-Bold.ttf";
    const char* semiBold = "assets/fonts/BarlowCondensed-SemiBold.ttf";
    for (const int size : {17, 22, 24, 29, 34, 48, 58, 77}) {
        manifest.push_back({Kind::Font, bold, Slot::Ball, Role::Hud, Weight::Bold, size});
    }
    for (const int size : {14, 19, 29}) {
        manifest.push_back({Kind::Font, semiBold, Slot::Ball, Role::Hud, Weight::SemiBold, size});
    }
    manifest.push_back({Kind::Font, "assets/fonts/BebasNeue-Regular.ttf", Slot::Ball, Role::Field,
                        Weight::Bold, Assets::kFieldFontSize});
    manifest.push_back({Kind::Font, "assets/fonts/JetBrainsMono-Regular.ttf", Slot::Ball,
                        Role::Debug, Weight::Bold, Assets::kDebugFontSize});

    // Play data.
    manifest.push_back({Kind::Plays, "data/plays"});
    manifest.push_back({Kind::Defense, "data/defense"});
    return manifest;
}

AssetLoader::AssetLoader(std::filesystem::path root, std::vector<ManifestEntry> manifest)
    : m_root(std::move(root)), m_manifest(std::move(manifest)) {}

AssetLoader::~AssetLoader() {
    m_stop = true;
    if (m_worker.joinable()) {
        m_worker.join();
    }
    for (Decoded& item : m_ready) { // decoded but never installed (closed during loading)
        freeDecoded(item.image);
        freeDecoded(item.font);
    }
}

void AssetLoader::start() {
#if !defined(__EMSCRIPTEN__)
    m_worker = std::thread(&AssetLoader::workerMain, this);
#endif
}

float AssetLoader::progress() const {
    return m_manifest.empty()
               ? 1.0f
               : static_cast<float>(m_installed) / static_cast<float>(m_manifest.size());
}

std::string AssetLoader::currentPath() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    return m_current;
}

// Worker thread: CPU work only. No raylib call made here touches OpenGL.
void AssetLoader::workerMain() {
    while (m_nextDecode < m_manifest.size() && !m_stop) {
        decodeNext();
    }
}

void AssetLoader::decodeNext() {
    const std::size_t index = m_nextDecode++;
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_current = m_manifest[index].path;
    }
    Decoded item = decode(index);
    std::lock_guard<std::mutex> lock(m_mutex);
    m_ready.push_back(std::move(item));
}

AssetLoader::Decoded AssetLoader::decode(std::size_t index) const {
    const ManifestEntry& entry = m_manifest[index];
    const std::filesystem::path path = m_root / entry.path;
    Decoded item;
    item.entry = index;
    switch (entry.kind) {
    case ManifestEntry::Kind::Texture:
        item.ok = decodeImage(path, item.image, item.error);
        break;
    case ManifestEntry::Kind::Font:
        item.ok = decodeFont(path, entry.pixelSize, item.font, item.error);
        break;
    case ManifestEntry::Kind::Plays:
    case ManifestEntry::Kind::Defense:
        item.playbook = loadPlaysFromDirectory(path);
        item.ok = !item.playbook.plays.empty();
        if (!item.ok) {
            item.error =
                item.playbook.errors.empty() ? "no play files found" : item.playbook.errors.front();
        }
        break;
    }
    return item;
}

// Main thread: GPU uploads happen here (see the class comment).
void AssetLoader::pump(Assets& assets, AppContext& context, int budget) {
#if defined(__EMSCRIPTEN__)
    if (m_nextDecode < m_manifest.size()) {
        decodeNext(); // no worker in the browser (see the class comment)
    }
#endif
    for (int i = 0; i < budget; ++i) {
        Decoded item;
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            if (m_ready.empty()) {
                return;
            }
            item = std::move(m_ready.front());
            m_ready.pop_front();
        }
        install(item, assets, context);
        ++m_installed;
    }
}

void AssetLoader::install(Decoded& item, Assets& assets, AppContext& context) {
    const ManifestEntry& entry = m_manifest[item.entry];
    if (!item.ok) {
        m_errors.push_back(entry.path + " (" + item.error + ")");
        if (entry.kind == ManifestEntry::Kind::Plays ||
            entry.kind == ManifestEntry::Kind::Defense) {
            m_fatal = true;
        }
        return;
    }
    switch (entry.kind) {
    case ManifestEntry::Kind::Texture: {
        const Texture2D texture = uploadTexture(item.image);
        switch (entry.slot) {
        case ManifestEntry::TextureSlot::PlayerAtlas:
            assets.setPlayerAtlas(texture);
            break;
        case ManifestEntry::TextureSlot::Ball:
            assets.setBallSprite(texture);
            break;
        case ManifestEntry::TextureSlot::Grass:
            assets.setGrassTiles(texture);
            break;
        }
        break;
    }
    case ManifestEntry::Kind::Font: {
        const Font font = uploadFont(item.font);
        switch (entry.role) {
        case ManifestEntry::FontRole::Hud:
            assets.addHudFont(entry.weight, entry.pixelSize, font);
            break;
        case ManifestEntry::FontRole::Field:
            assets.setFieldFont(font);
            break;
        case ManifestEntry::FontRole::Debug:
            assets.setDebugFont(font);
            break;
        }
        break;
    }
    case ManifestEntry::Kind::Plays:
        context.playbook = std::move(item.playbook.plays);
        break;
    case ManifestEntry::Kind::Defense:
        context.defense = std::move(item.playbook.plays.front());
        break;
    }
}

} // namespace gridiron
