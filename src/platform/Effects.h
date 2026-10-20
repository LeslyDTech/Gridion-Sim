#pragma once

#include "core/Vec2.h"
#include "core/World.h"

#include <raylib.h>

#include <array>
#include <cstddef>
#include <random>

namespace gridiron {

// Presentation-only state the simulation doesn't need: facing directions, the route overlay
// fade, camera shake and zoom and grass particles. (The "+N YDS" popup is part of the broadcast
// HUD: see BroadcastHud.)
//
// It is updated once per rendered frame by *watching* the const World (for example, the phase
// switching from Live to Over with a Tackled outcome triggers shake and particles), so the core
// knows nothing about any of it.
//
// No allocations: particles live in a fixed-size pool, and the oldest particle is recycled when
// the pool is full.
class Effects {
public:
    static constexpr std::size_t kMaxPlayers = 22;
    static constexpr std::size_t kParticlePoolSize = 256;

    Effects();

    // Call once per rendered frame, after the simulation steps have run.
    void update(const World& world, float frameSeconds);

    // Unit vector the player is facing (smoothed, so it doesn't flicker when they stop).
    Vec2 facing(int playerId) const;

    // Playbook overlay opacity: 1 before the snap, fading to 0 over half a second after it.
    float routeOpacity() const;

    // Camera: 1 = normal, < 1 = zoomed out while a deep pass is in the air.
    float zoomFactor() const { return m_zoom; }
    // Camera shake offset in yards (non-zero for a moment after a tackle).
    Vec2 shakeOffset() const;

    // World space: grass and chalk kicked up by tackles.
    void drawParticles() const;

private:
    struct Particle {
        Vec2 position;
        Vec2 velocity;
        float age = 0.0f;
        float lifetime = 0.0f; // 0 = slot unused
        float size = 0.0f;
        Color color{};
    };

    void onPlayEnded(const PlayOutcome& outcome);
    void spawnBurst(Vec2 at, int count, float speed);
    void updateFacing(const World& world, float frameSeconds);
    void updateParticles(float frameSeconds);
    void updateZoom(const World& world, float frameSeconds);

    std::array<Vec2, kMaxPlayers> m_facing{};
    std::array<Particle, kParticlePoolSize> m_particles{};
    std::size_t m_nextParticle = 0;
    std::mt19937 m_random{20261020}; // fixed seed: effects look the same every run

    PlayPhase m_previousPhase = PlayPhase::PreSnap;
    float m_secondsSinceSnap = 0.0f;
    float m_time = 0.0f;
    float m_trauma = 0.0f; // 0..1, shake strength is trauma^2
    float m_zoom = 1.0f;
};

} // namespace gridiron
