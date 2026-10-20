#include "FollowCamera.h"

#include "core/FieldGeometry.h"

#include <algorithm>
#include <cmath>

namespace gridiron {
namespace {

// How much area outside the field the camera may show (sideline area), in yards.
constexpr float kMargin = 4.0f;

// Higher = snappier. The camera closes ~1 - e^(-k*t) of the gap per t seconds, which
// behaves the same at any frame rate (unlike "move 10% per frame").
constexpr float kFollowSharpness = 6.0f;

// Clamp one axis so the view stays within [min - margin, max + margin]. If the view is wider
// than that range, center it instead.
float clampAxis(float center, float halfView, float fieldSize) {
    const float lo = -kMargin + halfView;
    const float hi = fieldSize + kMargin - halfView;
    return lo > hi ? fieldSize / 2.0f : std::clamp(center, lo, hi);
}

} // namespace

FollowCamera::FollowCamera(float pixelsPerYard) : m_baseZoom(pixelsPerYard) {
    m_camera.zoom = pixelsPerYard;
}

void FollowCamera::snapTo(Vec2 focus, int screenWidth, int screenHeight) {
    apply(clampToField(focus, screenWidth, screenHeight), {}, screenWidth, screenHeight);
}

void FollowCamera::follow(Vec2 focus, float frameSeconds, int screenWidth, int screenHeight,
                          float zoomFactor, Vec2 shake) {
    // Zoom is already smoothed by the caller (Effects), so apply it directly.
    m_camera.zoom = m_baseZoom * zoomFactor;
    const float t = 1.0f - std::exp(-kFollowSharpness * frameSeconds);
    const Vec2 target = clampToField(focus, screenWidth, screenHeight);
    apply(lerp(m_center, target, t), shake, screenWidth, screenHeight);
}

Vector2 FollowCamera::worldToScreen(Vec2 world) const {
    return GetWorldToScreen2D(Vector2{world.x, world.y}, m_camera);
}

Vec2 FollowCamera::clampToField(Vec2 center, int screenWidth, int screenHeight) const {
    const float halfViewX = static_cast<float>(screenWidth) / (2.0f * m_camera.zoom);
    const float halfViewY = static_cast<float>(screenHeight) / (2.0f * m_camera.zoom);
    return {clampAxis(center.x, halfViewX, field::kLength),
            clampAxis(center.y, halfViewY, field::kWidth)};
}

void FollowCamera::apply(Vec2 center, Vec2 shake, int screenWidth, int screenHeight) {
    m_center = center;
    const Vec2 shaken = center + shake;
    m_camera.target = Vector2{shaken.x, shaken.y};
    m_camera.offset =
        Vector2{static_cast<float>(screenWidth) / 2.0f, static_cast<float>(screenHeight) / 2.0f};
}

} // namespace gridiron
