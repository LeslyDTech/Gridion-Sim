#pragma once

#include "Vec2.h"

// Small, pure steering behaviors (after Craig Reynolds, "Steering Behaviors for Autonomous
// Characters"). Each returns a *desired velocity*; accelerateToward() then turns that into an
// actual velocity with limited acceleration, so players can't change direction instantly.
namespace gridiron::steering {

// Full speed straight at the target.
Vec2 seek(Vec2 position, Vec2 target, float maxSpeed);

// Like seek, but slows down linearly inside slowRadius and stops on the target.
Vec2 arrive(Vec2 position, Vec2 target, float maxSpeed, float slowRadius);

// Where to aim to cut off a moving target: its position after the time we'd need to reach it
// now, capped at maxLookahead seconds so a far-away target doesn't make us aim absurdly far ahead.
Vec2 predictInterceptPoint(Vec2 pursuerPosition, float pursuerSpeed, Vec2 targetPosition,
                           Vec2 targetVelocity, float maxLookahead);

// Seek toward the predicted intercept point.
Vec2 pursue(Vec2 position, float maxSpeed, Vec2 targetPosition, Vec2 targetVelocity,
            float maxLookahead);

// Moves velocity toward desiredVelocity by at most maxAcceleration * dt.
Vec2 accelerateToward(Vec2 velocity, Vec2 desiredVelocity, float maxAcceleration, float dt);

} // namespace gridiron::steering
