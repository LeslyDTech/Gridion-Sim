#pragma once

namespace gridiron {

class Assets;
class FollowCamera;

// Everything a draw call needs besides the thing being drawn, bundled so draw functions keep
// short signatures.
struct RenderContext {
    const FollowCamera& camera;
    const Assets& assets;
    float alpha;   // fixed-timestep interpolation factor: draw lerp(previous, current, alpha)
    bool polish;   // F2: false draws the plain Milestone 1 look, for before/after comparison
    double time;   // seconds since start, for pulsing/animated details
};

} // namespace gridiron
