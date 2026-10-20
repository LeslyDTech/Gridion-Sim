#include "Collision.h"

#include <cmath>

namespace gridiron {

SeparationShares separationShares(float massA, float massB) {
    const float total = massA + massB;
    if (total <= 0.0f) {
        return {0.5f, 0.5f};
    }
    return {massB / total, massA / total}; // the lighter body moves more
}

std::optional<Contact> findContact(const Circle& a, const Circle& b) {
    const Vec2 offset = b.center - a.center;
    const float radii = a.radius + b.radius;
    const float distSq = lengthSquared(offset);

    // Compare squared distances first: no square root for the common "not touching" case.
    if (distSq >= radii * radii) {
        return std::nullopt;
    }

    const float dist = std::sqrt(distSq);
    // Exactly on top of each other: any direction works, pick +x so the result is deterministic.
    const Vec2 normal = dist > 1e-6f ? offset / dist : Vec2{1.0f, 0.0f};
    return Contact{normal, radii - dist};
}

} // namespace gridiron
