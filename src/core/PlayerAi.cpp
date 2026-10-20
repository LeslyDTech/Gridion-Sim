#include "PlayerAi.h"

#include "Steering.h"

namespace gridiron::ai {
namespace {

constexpr float kMaxLookahead = 1.0f; // seconds a pursuer leads a moving target
constexpr float kBlockRange = 7.0f;   // blockers look for defenders this close (yards)
constexpr float kCoverCushion = 1.0f; // man defenders stay this far downfield of their man
constexpr float kArriveRadius = 1.5f; // slow down this close to a destination

const Player* ballCarrier(const Context& context) {
    if (context.ball.state != BallState::Held || context.ball.carrierId == Player::kNone) {
        return nullptr;
    }
    return &context.players[static_cast<std::size_t>(context.ball.carrierId)];
}

// Chase whatever the defense should be chasing: the carrier, or the spot a pass will land.
Vec2 chaseBall(const Player& self, const Context& context) {
    if (const Player* carrier = ballCarrier(context)) {
        return steering::pursue(self.position, self.attributes.maxSpeed, carrier->position,
                                carrier->velocity, kMaxLookahead);
    }
    if (context.ball.state == BallState::InAir) {
        return steering::seek(self.position, context.ball.target, self.attributes.maxSpeed);
    }
    return {};
}

// "Pursue" defenders read the play before committing: while the quarterback still holds the
// ball behind the line, they hold their zone instead of blitzing.
Vec2 pursueBehavior(const Player& self, const Context& context) {
    const Player* carrier = ballCarrier(context);
    const bool passStillPossible = carrier != nullptr && carrier->role == Role::QB &&
                                   carrier->position.x < context.lineOfScrimmageX;
    if (self.assignment == Assignment::Pursue && passStillPossible) {
        return steering::arrive(self.position, self.homePosition, self.attributes.maxSpeed,
                                kArriveRadius);
    }
    return chaseBall(self, context);
}

Vec2 coverBehavior(const Player& self, const Context& context) {
    if (self.coverTargetId == Player::kNone) {
        return chaseBall(self, context);
    }
    // Ball thrown at my man: break on the ball.
    if (context.ball.state == BallState::InAir &&
        context.ball.intendedReceiverId == self.coverTargetId) {
        return steering::seek(self.position, context.ball.target, self.attributes.maxSpeed);
    }
    const Player& receiver = context.players[static_cast<std::size_t>(self.coverTargetId)];
    const Vec2 shadowPoint = receiver.position + receiver.velocity * 0.3f + Vec2{kCoverCushion, 0};
    return steering::arrive(self.position, shadowPoint, self.attributes.maxSpeed, kArriveRadius);
}

// Nearest defender within kBlockRange, or nullptr.
const Player* findBlockTarget(const Player& self, const Context& context) {
    const Player* best = nullptr;
    float bestDistanceSq = kBlockRange * kBlockRange;
    for (const Player& other : context.players) {
        if (other.team != Team::Defense) {
            continue;
        }
        const float d = distanceSquared(self.position, other.position);
        if (d < bestDistanceSq) {
            bestDistanceSq = d;
            best = &other;
        }
    }
    return best;
}

// Step into the defender's path: aim for the point just in front of the defender on the line
// from the defender to the ball carrier. Collision does the actual "blocking".
Vec2 blockBehavior(const Player& self, const Context& context) {
    const Player* carrier = ballCarrier(context);
    const Player* defender = findBlockTarget(self, context);
    if (carrier == nullptr || defender == nullptr) {
        return {}; // hold ground
    }
    const Vec2 towardCarrier = normalized(carrier->position - defender->position);
    const float contactDistance = self.attributes.radius + defender->attributes.radius;
    const Vec2 interceptPoint = defender->position + towardCarrier * contactDistance;
    return steering::seek(self.position, interceptPoint, self.attributes.maxSpeed);
}

} // namespace

Vec2 desiredVelocity(const Player& self, const Context& context) {
    switch (self.state) {
    case PlayerState::Pursuing:
        return pursueBehavior(self, context);
    case PlayerState::Covering:
        return coverBehavior(self, context);
    case PlayerState::Blocking:
        return blockBehavior(self, context);
    case PlayerState::ReceivingPass:
        return steering::arrive(self.position, context.ball.target, self.attributes.maxSpeed, 0.5f);
    case PlayerState::PreSnap:
    case PlayerState::RunningRoute: // moved by World::runRoute
    case PlayerState::BallCarrier:  // moved by user input
    case PlayerState::Idle:
    case PlayerState::Tackled:
        break;
    }
    return {}; // stop
}

} // namespace gridiron::ai
