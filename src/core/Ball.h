#pragma once

#include "Player.h"
#include "Vec2.h"

#include <cstdint>

namespace gridiron {

enum class BallState : std::uint8_t {
    Spotted, // on the ground at the line of scrimmage, waiting for the snap
    Held,    // carried by carrierId
    InAir,   // a pass in flight toward `target`
    Dead,    // play over
};

// The football. A pass is a straight line from launch to target at constant ground speed. The
// arc is only visual (height()), which keeps catch logic purely 2D.
struct Ball {
    BallState state = BallState::Spotted;
    Vec2 position;
    Vec2 previousPosition; // for render interpolation
    int carrierId = Player::kNone;

    // Pass flight
    Vec2 launch;
    Vec2 target;
    float flightTime = 0.0f; // seconds from launch to target
    float elapsed = 0.0f;
    int intendedReceiverId = Player::kNone;

    // Visual height above the ground in yards, a parabola peaking mid-flight.
    float height() const;
};

// Starts a pass from `from` to `to` at `speed` yards/second.
void throwBall(Ball& ball, Vec2 from, Vec2 to, float speed, int receiverId);

// Advances a pass by dt. Returns true on the step the ball reaches its target.
bool advanceFlight(Ball& ball, float dt);

} // namespace gridiron
