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
constexpr float kZoomSharpness = 4.0f;

float smoothing(float sharpness, float seconds) { return 1.0f - std::exp(-sharpness * seconds); }

// Keep `center` within [lo, hi]: the range of centers for which the whole box is in view. If the
// box is bigger than the view (only when the user has zoomed in past the framing), the range is
// inverted: [hi, lo] is then the range that keeps the view inside the box, and the center is
// clamped to that instead, so it stays as close to the ball as possible.
float clampCenter(float center, float lo, float hi) {
    return lo > hi ? std::clamp(center, hi, lo) : std::clamp(center, lo, hi);
}

// Clamp one axis so the view stays within [-margin, fieldSize + margin]. If the view is wider
// than that range, center it instead.
float clampToField(float center, float halfView, float fieldSize, float margin) {
    const float lo = -margin + halfView;
    const float hi = fieldSize + margin - halfView;
    return lo > hi ? fieldSize / 2.0f : std::clamp(center, lo, hi);
}

} // namespace

FollowCamera::FollowCamera(float visibleYardsAcross) : m_visibleYardsAcross(visibleYardsAcross) {
    m_camera.zoom = 1280.0f / visibleYardsAcross; // replaced on the first frame
}

FollowCamera::View FollowCamera::safeView(int screenWidth, int screenHeight) const {
    return {std::max(1.0f, static_cast<float>(screenWidth) - 2.0f * m_safe.side),
            std::max(1.0f, static_cast<float>(screenHeight) - m_safe.top - m_safe.bottom)};
}

float FollowCamera::fitZoom(Bounds players, View view) const {
    const float width = std::max(players.max.x - players.min.x, 1.0f);
    const float height = std::max(players.max.y - players.min.y, 1.0f);
    return std::min(view.width / width, view.height / height);
}

Vec2 FollowCamera::constrainCenter(Vec2 center, Bounds players, View view) const {
    const float halfX = view.width / (2.0f * m_camera.zoom);
    const float halfY = view.height / (2.0f * m_camera.zoom);
    // Keep every player inside the view...
    Vec2 constrained{clampCenter(center.x, players.max.x - halfX, players.min.x + halfX),
                     clampCenter(center.y, players.max.y - halfY, players.min.y + halfY)};
    // ...and never show past the back of the end zones (or far beyond the sidelines).
    constrained.x = clampToField(constrained.x, halfX, field::kLength, kMarginX);
    constrained.y = clampToField(constrained.y, halfY, field::kWidth, kMarginY);
    return constrained;
}

void FollowCamera::snapTo(Vec2 focus, Bounds players, int screenWidth, int screenHeight) {
    const View view = safeView(screenWidth, screenHeight);
    const float closest = static_cast<float>(screenWidth) / m_visibleYardsAcross;
    m_camera.zoom = std::min(closest, fitZoom(players, view)) * m_userZoom;
    m_center = constrainCenter(focus, players, view);
    apply({}, screenWidth, view);
}

void FollowCamera::follow(Vec2 focus, Bounds players, float frameSeconds, int screenWidth,
                          int screenHeight, float zoomFactor, Vec2 shake) {
    const View view = safeView(screenWidth, screenHeight);
    const float closest = static_cast<float>(screenWidth) / m_visibleYardsAcross * zoomFactor;
    const float fit = fitZoom(players, view);

    // Smooth zoom changes, but never zoom in further than "everyone fits" allows (hard limit) --
    // unless the user has zoomed in on purpose, which scales both the target and the limit.
    const float targetZoom = std::min(closest, fit) * m_userZoom;
    m_camera.zoom += (targetZoom - m_camera.zoom) * smoothing(kZoomSharpness, frameSeconds);
    m_camera.zoom = std::min(m_camera.zoom, fit * std::max(m_userZoom, 1.0f));

    // Smooth the pan toward the ball, then enforce the framing rules (hard limits again).
    const Vec2 target = constrainCenter(focus, players, view);
    m_center = lerp(m_center, target, smoothing(kFollowSharpness, frameSeconds));
    m_center = constrainCenter(m_center, players, view);
    apply(shake, screenWidth, view);
}

Vector2 FollowCamera::worldToScreen(Vec2 world) const {
    return GetWorldToScreen2D(Vector2{world.x, world.y}, m_camera);
}

void FollowCamera::apply(Vec2 shake, int screenWidth, View view) {
    const Vec2 shaken = m_center + shake;
    m_camera.target = Vector2{shaken.x, shaken.y};
    // The camera's center is the center of the safe area, not of the window.
    m_camera.offset =
        Vector2{static_cast<float>(screenWidth) / 2.0f, m_safe.top + view.height / 2.0f};
}

} // namespace gridiron
