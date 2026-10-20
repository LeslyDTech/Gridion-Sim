#include "Ball.h"

#include <algorithm>

namespace gridiron {

float Ball::height() const {
    if (state != BallState::InAir || flightTime <= 0.0f) {
        return 0.0f;
    }
    const float t = elapsed / flightTime;
    const float peak = std::min(distance(launch, target) * 0.12f, 4.0f);
    return 4.0f * peak * t * (1.0f - t); // 0 at both ends, `peak` at t = 0.5
}

void throwBall(Ball& ball, Vec2 from, Vec2 to, float speed, int receiverId) {
    ball.state = BallState::InAir;
    ball.carrierId = Player::kNone;
    ball.launch = from;
    ball.target = to;
    ball.position = from;
    ball.elapsed = 0.0f;
    ball.flightTime = std::max(distance(from, to) / speed, 0.1f);
    ball.intendedReceiverId = receiverId;
}

bool advanceFlight(Ball& ball, float dt) {
    ball.elapsed = std::min(ball.elapsed + dt, ball.flightTime);
    ball.position = lerp(ball.launch, ball.target, ball.elapsed / ball.flightTime);
    return ball.elapsed >= ball.flightTime;
}

} // namespace gridiron
