#include "PlayerSprites.h"

#include "Theme.h"
#include "core/World.h"

#include <cmath>

namespace gridiron::sprite {
namespace {

constexpr int kPositions = 9;
constexpr int kPoses = 8;
constexpr int kDirections = 4;

constexpr float kThrowPoseSeconds = 0.3f;  // after the release
constexpr float kCatchPoseSeconds = 0.35f; // before the ball arrives
constexpr float kTackleReachYards = 1.6f;  // defender this close to the carrier
constexpr float kRunningIntensity = 0.2f;  // stride intensity above which the run pose shows

} // namespace

Position positionFor(Role role) {
    switch (role) {
    case Role::QB:
        return Position::QB;
    case Role::HB:
    case Role::FB:
        return Position::RB;
    case Role::WR:
        return Position::WR;
    case Role::TE:
        return Position::TE;
    case Role::OL:
        return Position::OL;
    case Role::DL:
        return Position::DL;
    case Role::LB:
        return Position::LB;
    case Role::CB:
    case Role::S:
        break;
    }
    return Position::DB;
}

Pose poseFor(const Player& player, const World& world, float strideIntensity) {
    const Ball& ball = world.ball();
    if (player.state == PlayerState::Tackled) {
        return Pose::Fall;
    }
    if (player.state == PlayerState::PreSnap) {
        return Pose::Idle;
    }
    if (ball.state == BallState::InAir) {
        if (player.role == Role::QB && player.team == Team::Offense &&
            ball.elapsed < kThrowPoseSeconds) {
            return Pose::Throw;
        }
        if (player.id == ball.intendedReceiverId &&
            ball.flightTime - ball.elapsed < kCatchPoseSeconds) {
            return Pose::Catch;
        }
    }
    if (player.state == PlayerState::Blocking || player.engaged) {
        return Pose::Block;
    }
    if (player.team == Team::Defense && ball.state == BallState::Held &&
        ball.carrierId != Player::kNone) {
        const Player& carrier = world.players()[static_cast<std::size_t>(ball.carrierId)];
        if (carrier.team == Team::Offense && distanceSquared(player.position, carrier.position) <
                                                 kTackleReachYards * kTackleReachYards) {
            return Pose::Tackle;
        }
    }
    return strideIntensity > kRunningIntensity ? Pose::Run : Pose::Idle;
}

Facing facingFor(Vec2 facing) {
    // Screen y points down, so +90 degrees is "down".
    const float angle = std::atan2(facing.y, facing.x) * RAD2DEG; // -180..180
    if (angle >= -45.0f && angle < 45.0f) {
        return {Direction::Right, angle};
    }
    if (angle >= 45.0f && angle < 135.0f) {
        return {Direction::Down, angle - 90.0f};
    }
    if (angle >= -135.0f && angle < -45.0f) {
        return {Direction::Up, angle + 90.0f};
    }
    return {Direction::Left, angle >= 0.0f ? angle - 180.0f : angle + 180.0f};
}

Rectangle tile(Team team, Position position, Pose pose, Direction direction) {
    using namespace theme::sprite;
    const int teamIndex = team == Team::Offense ? 0 : 1; // Comets, then Blaze
    const int index =
        ((teamIndex * kPositions + static_cast<int>(position)) * kPoses + static_cast<int>(pose)) *
            kDirections +
        static_cast<int>(direction);
    return Rectangle{static_cast<float>(index % kTilesPerRow) * kTilePitch,
                     static_cast<float>(index / kTilesPerRow) * kTilePitch, kTileSize, kTileSize};
}

} // namespace gridiron::sprite
