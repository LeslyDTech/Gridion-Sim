#pragma once

namespace gridiron {

class FollowCamera;

// Stateless drawing functions for the world. Shapes are drawn in world units (yards) inside the
// camera's 2D mode; text is drawn afterwards in screen space so it stays crisp at any zoom.
namespace render {

// Turf, stripes, end zones, boundary lines, yard lines and hash marks (world space).
void drawFieldSurface();

// Yard-line numbers (screen space, call after EndMode2D).
void drawFieldNumbers(const FollowCamera& camera);

} // namespace render
} // namespace gridiron
