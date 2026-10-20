#pragma once

#include <cmath>

namespace gridiron {

// A plain 2D vector in world space. Units are yards (positions) or yards/second (velocities).
//
// This is deliberately our own type rather than raylib's Vector2: the core library must not
// depend on raylib, and keeping math in one small value type makes it trivial to unit test.
struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;

    constexpr Vec2& operator+=(Vec2 other) {
        x += other.x;
        y += other.y;
        return *this;
    }
    constexpr Vec2& operator-=(Vec2 other) {
        x -= other.x;
        y -= other.y;
        return *this;
    }
    constexpr Vec2& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        return *this;
    }
};

constexpr Vec2 operator+(Vec2 a, Vec2 b) { return {a.x + b.x, a.y + b.y}; }
constexpr Vec2 operator-(Vec2 a, Vec2 b) { return {a.x - b.x, a.y - b.y}; }
constexpr Vec2 operator-(Vec2 v) { return {-v.x, -v.y}; }
constexpr Vec2 operator*(Vec2 v, float s) { return {v.x * s, v.y * s}; }
constexpr Vec2 operator*(float s, Vec2 v) { return {v.x * s, v.y * s}; }
constexpr Vec2 operator/(Vec2 v, float s) { return {v.x / s, v.y / s}; }
constexpr bool operator==(Vec2 a, Vec2 b) { return a.x == b.x && a.y == b.y; }
constexpr bool operator!=(Vec2 a, Vec2 b) { return !(a == b); }

constexpr float dot(Vec2 a, Vec2 b) { return a.x * b.x + a.y * b.y; }
constexpr float lengthSquared(Vec2 v) { return dot(v, v); }
inline float length(Vec2 v) { return std::sqrt(lengthSquared(v)); }
constexpr float distanceSquared(Vec2 a, Vec2 b) { return lengthSquared(b - a); }
inline float distance(Vec2 a, Vec2 b) { return length(b - a); }

// Linear interpolation: t = 0 returns a, t = 1 returns b.
constexpr Vec2 lerp(Vec2 a, Vec2 b, float t) { return a + (b - a) * t; }

// Unit-length copy of v, or the zero vector if v is (nearly) zero. Returning zero instead of
// dividing by zero means callers never have to special-case "already at the target".
inline Vec2 normalized(Vec2 v) {
    const float len = length(v);
    return len > 1e-6f ? v / len : Vec2{};
}

// Shortens v to at most maxLength, keeping its direction.
inline Vec2 clampLength(Vec2 v, float maxLength) {
    const float lenSq = lengthSquared(v);
    if (lenSq <= maxLength * maxLength) {
        return v;
    }
    return v * (maxLength / std::sqrt(lenSq));
}

} // namespace gridiron
