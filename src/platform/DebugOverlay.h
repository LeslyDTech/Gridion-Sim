#pragma once

namespace gridiron {

class Assets;
class SpatialHash;

// What App measured during one rendered frame.
struct FrameSample {
    float frameSeconds = 0.0f;
    double updateSeconds = 0.0; // CPU time spent in fixed simulation steps this frame
    int steps = 0;              // fixed steps run this frame (0, 1, 2... depending on frame rate)
    int collisionChecks = 0;    // narrow-phase tests in the last step
};

// Settings shown alongside the measurements.
struct OverlaySettings {
    int fpsCap = 0; // 0 = uncapped
    bool polish = true;
    int players = 0;
};

// The F1 debug view: performance readout, broad-phase grid and player states. Off by default.
//
// Readability rules: the numbers are averaged over a 250 ms window and refreshed at 4 Hz (a value
// that changes every frame can't be read), and they are drawn in a monospace font with fixed-width
// fields, so a value changing from 9 to 10 never shifts the text after it.
class DebugOverlay {
public:
    static constexpr float kRefreshSeconds = 0.25f; // 4 Hz
    // Screen space the stats panel occupies on the left, so the HUD can avoid it.
    static constexpr int kPanelRight = 400;

    void toggle() { m_enabled = !m_enabled; }
    bool enabled() const { return m_enabled; }

    // Call once per rendered frame (also while hidden, so values are current when shown).
    void record(const FrameSample& sample);

    // World space (inside BeginMode2D): grid lines, occupied cells tinted by crowding.
    void drawGrid(const SpatialHash& grid) const;

    // Screen space (after EndMode2D).
    void drawStats(const Assets& assets, const OverlaySettings& settings) const;

private:
    // Totals for the window being measured.
    struct Window {
        float seconds = 0.0f;
        int frames = 0;
        double updateSeconds = 0.0;
        int steps = 0;
        long long collisionChecks = 0;
    };
    // Averages of the last completed window: what is on screen.
    struct Shown {
        float fps = 0.0f;
        float updateMicroseconds = 0.0f; // per fixed step
        float stepsPerFrame = 0.0f;
        float collisionChecks = 0.0f;
    };

    bool m_enabled = false;
    Window m_window;
    Shown m_shown;
};

} // namespace gridiron
