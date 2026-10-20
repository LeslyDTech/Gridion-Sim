#pragma once

#include "Vec2.h"

#include <algorithm>

// Field dimensions and helpers. All values are in yards.
//
// World coordinate system:
//   x runs along the field, 0 = back of the offense's own end zone, 120 = back of the
//     opponent's end zone. The offense always drives toward +x.
//   y runs across the field, 0 = one sideline, kWidth = the other.
// On screen, x grows to the right and y grows downward.
namespace gridiron::field {

inline constexpr float kLength = 120.0f;       // 100 yards + two 10-yard end zones
inline constexpr float kWidth = 160.0f / 3.0f; // 160 feet = 53 1/3 yards
inline constexpr float kEndZoneDepth = 10.0f;
inline constexpr float kOwnGoalLine = kEndZoneDepth;                // x = 10
inline constexpr float kOpponentGoalLine = kLength - kEndZoneDepth; // x = 110
inline constexpr float kCenterY = kWidth / 2.0f;

// NFL hash marks sit 70 ft 9 in from each sideline (the college ones are wider).
inline constexpr float kHashFromSideline = (70.0f * 12.0f + 9.0f) / 36.0f; // ~23.58 yards
inline constexpr float kNearHashY = kHashFromSideline;
inline constexpr float kFarHashY = kWidth - kHashFromSideline;

// x of the 25-yard line on the offense's own side, where every drive starts.
inline constexpr float kStartingLineOfScrimmageX = kOwnGoalLine + 25.0f;

constexpr bool isInBounds(Vec2 p) {
    return p.x >= 0.0f && p.x <= kLength && p.y >= 0.0f && p.y <= kWidth;
}

constexpr bool isInOpponentEndZone(Vec2 p) { return p.x >= kOpponentGoalLine; }
constexpr bool isInOwnEndZone(Vec2 p) { return p.x <= kOwnGoalLine; }

// Football yard-line number for a world x (e.g. x = 35 -> 25, x = 60 -> 50, x = 85 -> 25).
inline int yardLineNumber(float x) {
    const float fromOwnGoal = std::clamp(x - kOwnGoalLine, 0.0f, 100.0f);
    const float nearestGoal = std::min(fromOwnGoal, 100.0f - fromOwnGoal);
    return static_cast<int>(nearestGoal + 0.5f);
}

} // namespace gridiron::field
