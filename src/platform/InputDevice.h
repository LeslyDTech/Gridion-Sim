#pragma once

#include "core/InputCommands.h"

namespace gridiron {

// App-level commands that never reach the simulation (debug views, frame-rate cap).
struct AppCommands {
    bool toggleDebug = false;
    bool cycleFpsCap = false;
};

// The single input abstraction: merges keyboard and the first gamepad into device-independent
// commands.
//
// Edge events ("pressed this frame") need care with a fixed timestep: a rendered frame can run
// zero simulation steps (high refresh rate) or several (after a hitch). poll() therefore *latches*
// edges until the next consumeForStep(), so a quick tap is never lost and never handled twice.
class InputDevice {
public:
    // Call once per rendered frame, before running simulation steps.
    void poll();

    // Commands for one simulation step. Edge events are cleared after the first call.
    InputCommands consumeForStep();

    // App commands for this rendered frame.
    const AppCommands& appCommands() const { return m_app; }

private:
    InputCommands m_pending;
    AppCommands m_app;
};

} // namespace gridiron
