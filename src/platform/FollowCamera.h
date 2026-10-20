#pragma once

#include "core/Vec2.h"

#include <raylib.h>

namespace gridiron {

// A 2D camera that smoothly follows a focus point (the ball) and never shows much beyond the
// field. Wraps raylib's Camera2D so the renderer can draw directly in yards: inside
// BeginMode2D(camera.raylibCamera()), one unit is one yard.
class FollowCamera {
public:
    explicit FollowCamera(float pixelsPerYard);

    // Jump straight to a point (used when a new play is set up).
    void snapTo(Vec2 focus, int screenWidth, int screenHeight);

    // Ease toward focus. Called once per rendered frame with the real frame time.
    //   zoomFactor: 1 = normal, < 1 = zoomed out (deep passes).
    //   shake:      offset in yards added after clamping, so shake works at the field edges too.
    void follow(Vec2 focus, float frameSeconds, int screenWidth, int screenHeight,
                float zoomFactor = 1.0f, Vec2 shake = {});

    // Pixels at the bottom of the screen covered by HUD (the scorebug). The camera centers the
    // field in the area above it, so no player is ever hidden behind the HUD.
    void setBottomInset(int pixels) { m_bottomInset = pixels; }

    const Camera2D& raylibCamera() const { return m_camera; }

    // World (yards) -> screen (pixels), for text drawn on top of the world.
    Vector2 worldToScreen(Vec2 world) const;
    float pixelsPerYard() const { return m_camera.zoom; }

private:
    Vec2 clampToField(Vec2 center, int screenWidth, int screenHeight) const;
    void apply(Vec2 center, Vec2 shake, int screenWidth, int screenHeight);

    Camera2D m_camera{};
    float m_baseZoom;
    int m_bottomInset = 0;
    Vec2 m_center;
};

} // namespace gridiron
