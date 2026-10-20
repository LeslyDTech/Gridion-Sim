#pragma once

#include "Vec2.h"

namespace gridiron {

// Everything the simulation needs from the player for one fixed update, independent of device.
//
// The platform layer fills this from keyboard and/or gamepad (see platform/InputDevice), so the
// core never knows which device was used. That also makes it trivial to drive the simulation from
// tests or from a replay file.
struct InputCommands {
    // Desired movement direction for the user-controlled ball carrier. Length is in [0, 1]:
    // 1 means full speed, a half-pushed analog stick gives 0.5.
    Vec2 move;

    // "Pressed this step" events. They fire once, never while a button is held.
    bool snap = false;
    bool nextPlay = false;
    int throwTarget = -1; // receiver number (0-based) the user wants to throw to, -1 = none

    static constexpr int kNoThrow = -1;
};

} // namespace gridiron
