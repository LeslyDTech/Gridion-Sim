#include "Steering.h"

#include <algorithm>

namespace gridiron::steering {

Vec2 seek(Vec2 position, Vec2 target, float maxSpeed) {
    return normalized(target - position) * maxSpeed;
}

Vec2 arrive(Vec2 position, Vec2 target, float maxSpeed, float slowRadius) {
    const Vec2 offset = target - position;
    const float dist = length(offset);
    if (dist < 1e-4f) {
        return {};
    }
    const float speed = dist < slowRadius ? maxSpeed * (dist / slowRadius) : maxSpeed;
    return offset * (speed / dist);
}

Vec2 predictInterceptPoint(Vec2 pursuerPosition, float pursuerSpeed, Vec2 targetPosition,
                           Vec2 targetVelocity, float maxLookahead) {
    if (pursuerSpeed <= 0.0f) {
        return targetPosition;
    }
    const float timeToReach = distance(pursuerPosition, targetPosition) / pursuerSpeed;
    const float lookahead = std::min(timeToReach, maxLookahead);
    return targetPosition + targetVelocity * lookahead;
}

Vec2 pursue(Vec2 position, float maxSpeed, Vec2 targetPosition, Vec2 targetVelocity,
            float maxLookahead) {
    const Vec2 aim =
        predictInterceptPoint(position, maxSpeed, targetPosition, targetVelocity, maxLookahead);
    return seek(position, aim, maxSpeed);
}

Vec2 accelerateToward(Vec2 velocity, Vec2 desiredVelocity, float maxAcceleration, float dt) {
    const Vec2 change = clampLength(desiredVelocity - velocity, maxAcceleration * dt);
    return velocity + change;
}

} // namespace gridiron::steering
