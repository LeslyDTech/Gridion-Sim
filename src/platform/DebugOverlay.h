#pragma once

namespace gridiron {

class SpatialHash;

// Numbers measured by App each frame and shown in the top-left corner.
struct FrameStats {
    int fps = 0;
    int fpsCap = 0;         // 0 = uncapped
    double updateMs = 0.0;  // smoothed CPU time of one fixed simulation step
    int stepsThisFrame = 0; // fixed steps run during the last rendered frame
    int collisionChecks = 0;
    int bruteForcePairs = 0; // what checking every pair would cost, for comparison
};

// The always-on performance readout plus the F1 debug view (grid, collision counter, states).
class DebugOverlay {
public:
    void toggle() { m_enabled = !m_enabled; }
    bool enabled() const { return m_enabled; }

    // World space (inside BeginMode2D): grid lines, occupied cells tinted by crowding.
    void drawGrid(const SpatialHash& grid) const;

    // Screen space (after EndMode2D).
    void drawStats(const FrameStats& stats) const;

private:
    bool m_enabled = false;
};

} // namespace gridiron
