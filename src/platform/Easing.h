#pragma once

#include <algorithm>

// Standard easing curves (see easings.net). Input t is clamped to [0, 1]; output is 0 at t = 0
// and 1 at t = 1.
namespace gridiron::easing {

inline float clamp01(float t) { return std::clamp(t, 0.0f, 1.0f); }

// Fast start, gentle stop.
inline float outCubic(float t) {
    const float u = 1.0f - clamp01(t);
    return 1.0f - u * u * u;
}

// Overshoots slightly past 1, then settles: gives a "pop" when something appears.
inline float outBack(float t) {
    constexpr float c1 = 1.70158f;
    constexpr float c3 = c1 + 1.0f;
    const float u = clamp01(t) - 1.0f;
    return 1.0f + c3 * u * u * u + c1 * u * u;
}

} // namespace gridiron::easing
