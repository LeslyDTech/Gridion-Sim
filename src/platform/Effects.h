#pragma once

#include "core/Vec2.h"
#include "core/World.h"

#include <array>

namespace gridiron {

// Presentation-only state that the simulation doesn't need: which way each player is facing.
// It is updated once per rendered frame by watching the (const) World, so the core stays
// unaware of any of it.
class Effects {
public:
    static constexpr std::size_t kMaxPlayers = 22;

    Effects();

    // Call once per rendered frame, after the simulation steps have run.
    void update(const World& world, float frameSeconds);

    // Unit vector the player is facing (smoothed, so it doesn't flicker when they stop).
    Vec2 facing(int playerId) const;

    // Playbook overlay opacity: 1 before the snap, fading to 0 over half a second after it.
    float routeOpacity() const;

private:
    void updateFacing(const World& world, float frameSeconds);

    std::array<Vec2, kMaxPlayers> m_facing{};
    float m_secondsSinceSnap = 0.0f;
};

} // namespace gridiron
