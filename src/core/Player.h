#pragma once

#include "PlayerState.h"
#include "Vec2.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace gridiron {

enum class Team : std::uint8_t { Offense, Defense };

enum class Role : std::uint8_t { QB, HB, FB, WR, TE, OL, DL, LB, CB, S };

// Physical traits shared by every player of a role. Speeds are yards/second: a 4.4 s 40-yard
// dash is roughly 9 yd/s top speed.
struct PlayerAttributes {
    float maxSpeed = 7.0f;
    float acceleration = 12.0f; // yards/second^2
    float radius = 0.55f;       // collision circle, yards
    float mass = 1.0f;          // relative weight: heavier players are pushed less in contact
    // Ratings, 0-1.
    float blocking = 0.5f; // how long a block holds a defender
    float shedding = 0.5f; // how quickly a defender gets off a block
    float tackling = 0.5f; // chance of bringing the ball carrier down on contact
};

constexpr PlayerAttributes attributesFor(Role role) {
    switch (role) {
    //                 speed  accel  radius mass   block  shed   tackle
    case Role::QB:
        return {7.6f, 12.0f, 0.55f, 1.00f, 0.30f, 0.30f, 0.30f};
    case Role::HB:
        return {8.8f, 14.0f, 0.55f, 1.00f, 0.50f, 0.50f, 0.40f};
    case Role::FB:
        return {7.8f, 12.0f, 0.60f, 1.20f, 0.80f, 0.50f, 0.50f};
    case Role::WR:
        return {9.2f, 14.0f, 0.50f, 0.85f, 0.40f, 0.40f, 0.30f};
    case Role::TE:
        return {8.2f, 12.0f, 0.60f, 1.20f, 0.75f, 0.50f, 0.50f};
    case Role::OL:
        return {6.4f, 9.0f, 0.65f, 1.50f, 1.00f, 0.50f, 0.30f};
    case Role::DL:
        return {7.0f, 10.0f, 0.65f, 1.40f, 0.50f, 0.80f, 0.70f};
    case Role::LB:
        return {8.2f, 12.0f, 0.60f, 1.15f, 0.50f, 0.65f, 0.85f};
    case Role::CB:
        return {9.0f, 14.0f, 0.50f, 0.85f, 0.30f, 0.45f, 0.65f};
    case Role::S:
        return {8.8f, 13.0f, 0.55f, 0.95f, 0.30f, 0.50f, 0.75f};
    }
    return {};
}

// One player on the field. A plain value: the World owns all 22 in a std::vector<Player> and
// refers to players by index (id), never by pointer, so the vector can be rebuilt freely.
struct Player {
    static constexpr int kNone = -1;

    int id = kNone; // index in World::players()
    Team team = Team::Offense;
    Role role = Role::WR;
    Assignment assignment = Assignment::None;
    int jersey = 0;
    PlayerAttributes attributes;

    PlayerState state = PlayerState::PreSnap;
    Vec2 position;
    Vec2 previousPosition; // position at the previous fixed step, for render interpolation
    Vec2 velocity;
    Vec2 homePosition; // where the player lined up; zone defenders hold near it

    std::vector<Vec2> route; // absolute field waypoints (empty if not a route runner)
    std::size_t nextWaypoint = 0;

    // Blocking engagement (defenders only). While engaged a defender is held to a crawl; after
    // shedding the block he slips free for a moment and can't be re-engaged.
    bool engaged = false;        // in contact with a blocker this step
    float engagedSeconds = 0.0f; // how long the current block has held
    float shedSeconds = 0.0f;    // > 0: just shed a block, free to move for this long
    // > 0: just missed a tackle; stumbling, and can't try again until this runs out.
    float missedTackleSeconds = 0.0f;

    int blockTargetId = kNone;  // blockers: the defender assigned this step (World::assignBlocks)
    int coverTargetId = kNone;  // man-coverage receiver's id
    int receiverNumber = kNone; // 0-based throw button for eligible receivers

    void handle(PlayerEvent event) { state = nextState(state, event, assignment); }
};

} // namespace gridiron
