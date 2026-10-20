#pragma once

#include "core/Vec2.h"

#include <raylib.h>

namespace gridiron {

// Axis-aligned box in world space (yards).
struct Bounds {
    Vec2 min;
    Vec2 max;
};

// Screen margins (pixels) the HUD covers. Players are kept out of them.
struct SafeArea {
    float top = 0.0f;    // banner band
    float bottom = 0.0f; // scorebug + controls bar
    float side = 0.0f;
};

// A 2D camera that follows the ball while keeping every player inside the HUD-free part of the
// screen. Wraps raylib's Camera2D so the renderer can draw directly in yards: inside
// BeginMode2D(camera.raylibCamera()), one unit is one yard.
//
// Framing rules, in priority order:
//   1. All players (the `players` box) are inside the safe area: the camera zooms out as far as
//      needed and is clamped so the box fits. Enforced every frame after smoothing, so a fast
//      player can never slip under the HUD while the camera catches up.
//   2. Zoom is never closer than `visibleYardsAcross` (times the effects zoom factor).
//   3. Within that, the camera centers on the ball, and never shows past the back of an end zone.
class FollowCamera {
public:
    // `visibleYardsAcross`: how many yards fit across the window at the closest zoom.
    explicit FollowCamera(float visibleYardsAcross);

    void setSafeArea(SafeArea area) { m_safe = area; }

    // Jump straight to the framing (used when a new play is set up).
    void snapTo(Vec2 focus, Bounds players, int screenWidth, int screenHeight);

    // Ease toward the framing. Called once per rendered frame with the real frame time.
    //   zoomFactor: 1 = normal, < 1 = zoomed out (deep passes).
    //   shake:      offset in yards added last, so shake works at the field edges too.
    void follow(Vec2 focus, Bounds players, float frameSeconds, int screenWidth, int screenHeight,
                float zoomFactor = 1.0f, Vec2 shake = {});

    const Camera2D& raylibCamera() const { return m_camera; }

    // World (yards) -> screen (pixels), for text drawn on top of the world.
    Vector2 worldToScreen(Vec2 world) const;
    float pixelsPerYard() const { return m_camera.zoom; }

private:
    struct View {
        float width; // safe area size in pixels
        float height;
    };

    View safeView(int screenWidth, int screenHeight) const;
    float fitZoom(Bounds players, View view) const;
    Vec2 constrainCenter(Vec2 center, Bounds players, View view) const;
    void apply(Vec2 shake, int screenWidth, View view);

    Camera2D m_camera{};
    float m_visibleYardsAcross;
    SafeArea m_safe;
    Vec2 m_center;
};

} // namespace gridiron
