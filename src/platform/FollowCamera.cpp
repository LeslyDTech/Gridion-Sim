#include "FollowCamera.h"

#include "core/FieldGeometry.h"

#include <algorithm>
#include <cmath>

namespace gridiron {
namespace {

// How far past the field the camera may show, in yards. Along the field: nothing beyond the back
// of either end zone. Across it: a strip of sideline, so players on the sideline stay visible.
constexpr float kMarginX = 0.0f;
constexpr float kMarginY = 4.0f;

// Higher = snappier. The camera closes ~1 - e^(-k*t) of the gap per t seconds, which
// behaves the same at any frame rate (unlike "move 10% per frame").
constexpr float kFollowSharpness = 6.0f;

// Clamp one axis so the view stays within [-margin, fieldSize + margin]. If the view is wider
// than that range, center it instead.
float clampAxis(float center, float halfView, float fieldSize, float margin) {
    const float lo = -margin + halfView;
    const float hi = fieldSize + margin - halfView;
    return lo > hi ? fieldSize / 2.0f : std::clamp(center, lo, hi);
}

} // namespace

FollowCamera::FollowCamera(float visibleYardsAcross) : m_visibleYardsAcross(visibleYardsAcross) {
    m_camera.zoom = 1280.0f / visibleYardsAcross; // replaced on the first frame
}

float FollowCamera::baseZoom(int screenWidth) const {
    // Derived from the window width every frame, so resizing the window keeps ~32 yards across.
    return static_cast<float>(screenWidth) / m_visibleYardsAcross;
}

void FollowCamera::snapTo(Vec2 focus, int screenWidth, int screenHeight) {
    screenHeight -= m_bottomInset;
    m_camera.zoom = baseZoom(screenWidth);
    apply(clampToField(focus, screenWidth, screenHeight), {}, screenWidth, screenHeight);
}

void FollowCamera::follow(Vec2 focus, float frameSeconds, int screenWidth, int screenHeight,
                          float zoomFactor, Vec2 shake) {
    screenHeight -= m_bottomInset; // only the area above the HUD counts as the view
    // Zoom is already smoothed by the caller (Effects), so apply it directly.
    m_camera.zoom = baseZoom(screenWidth) * zoomFactor;
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
    return {clampAxis(center.x, halfViewX, field::kLength, kMarginX),
            clampAxis(center.y, halfViewY, field::kWidth, kMarginY)};
}

void FollowCamera::apply(Vec2 center, Vec2 shake, int screenWidth, int screenHeight) {
    m_center = center;
    const Vec2 shaken = center + shake;
    m_camera.target = Vector2{shaken.x, shaken.y};
    m_camera.offset =
        Vector2{static_cast<float>(screenWidth) / 2.0f, static_cast<float>(screenHeight) / 2.0f};
}

} // namespace gridiron
