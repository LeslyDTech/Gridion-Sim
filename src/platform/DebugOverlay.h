#pragma once

namespace gridiron {

// Numbers measured by App each frame and shown in the top-left corner.
struct FrameStats {
    int fps = 0;
    int fpsCap = 0;              // 0 = uncapped
    double updateMs = 0.0;       // smoothed CPU time of one fixed simulation step
    int stepsThisFrame = 0;      // fixed steps run during the last rendered frame
};

// The always-on performance readout plus the F1 debug view toggle.
class DebugOverlay {
public:
    void toggle() { m_enabled = !m_enabled; }
    bool enabled() const { return m_enabled; }

    // Screen-space text. Call after EndMode2D.
    void drawStats(const FrameStats& stats) const;

private:
    bool m_enabled = false;
};

} // namespace gridiron
