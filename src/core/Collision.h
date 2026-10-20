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

// How much of an overlap each body takes when two bodies are pushed apart: shares are inversely
// proportional to mass, so a heavy lineman barely moves while a light receiver is shoved aside.
// The two shares always add up to 1.
struct SeparationShares {
    float a;
    float b;
};
SeparationShares separationShares(float massA, float massB);

// Narrow phase: the exact test for one candidate pair from the broad phase.
// Returns nothing if the circles don't overlap.
std::optional<Contact> findContact(const Circle& a, const Circle& b);

} // namespace gridiron
