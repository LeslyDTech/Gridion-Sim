#pragma once

#include "Vec2.h"

#include <optional>

namespace gridiron {

struct Circle {
    Vec2 center;
    float radius = 0.0f;
};

// How two overlapping circles touch.
struct Contact {
    Vec2 normal;       // unit vector pointing from a's center toward b's center
    float penetration; // overlap depth in yards (> 0)
};

// Narrow phase: the exact test for one candidate pair from the broad phase.
// Returns nothing if the circles don't overlap.
std::optional<Contact> findContact(const Circle& a, const Circle& b);

} // namespace gridiron
