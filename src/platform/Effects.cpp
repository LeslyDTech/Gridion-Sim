#include "Effects.h"

#include <algorithm>
#include <cmath>

namespace gridiron {
namespace {

constexpr float kTurnSharpness = 12.0f; // how quickly the facing indicator follows velocity
constexpr float kMinTurnSpeed = 1.0f;   // below this speed (yd/s) keep the old facing
constexpr float kRouteFadeSeconds = 0.5f;

// Exponential smoothing factor that behaves the same at any frame rate.
float smoothing(float sharpness, float seconds) { return 1.0f - std::exp(-sharpness * seconds); }

Vec2 defaultFacing(Team team) {
    return team == Team::Offense ? Vec2{1.0f, 0.0f} : Vec2{-1.0f, 0.0f};
}

} // namespace

Effects::Effects() { m_facing.fill(Vec2{1.0f, 0.0f}); }

void Effects::update(const World& world, float frameSeconds) {
    m_secondsSinceSnap =
        world.phase() == PlayPhase::PreSnap ? 0.0f : m_secondsSinceSnap + frameSeconds;
    updateFacing(world, frameSeconds);
}

float Effects::routeOpacity() const {
    return std::clamp(1.0f - m_secondsSinceSnap / kRouteFadeSeconds, 0.0f, 1.0f);
}

Vec2 Effects::facing(int playerId) const {
    const auto index = static_cast<std::size_t>(playerId);
    return index < m_facing.size() ? m_facing[index] : Vec2{1.0f, 0.0f};
}

void Effects::updateFacing(const World& world, float frameSeconds) {
    const float t = smoothing(kTurnSharpness, frameSeconds);
    for (const Player& player : world.players()) {
        const auto index = static_cast<std::size_t>(player.id);
        if (index >= m_facing.size()) {
            continue;
        }
        Vec2& facing = m_facing[index];
        if (world.phase() == PlayPhase::PreSnap) {
            facing = defaultFacing(player.team); // lined up looking across the ball
        } else if (lengthSquared(player.velocity) > kMinTurnSpeed * kMinTurnSpeed) {
            facing = normalized(lerp(facing, normalized(player.velocity), t));
        }
    }
}

} // namespace gridiron
