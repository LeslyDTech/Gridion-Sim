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
};

constexpr PlayerAttributes attributesFor(Role role) {
    switch (role) {
    case Role::QB:
        return {7.6f, 12.0f, 0.55f};
    case Role::HB:
        return {8.8f, 14.0f, 0.55f};
    case Role::FB:
        return {7.8f, 12.0f, 0.6f};
    case Role::WR:
        return {9.2f, 14.0f, 0.5f};
    case Role::TE:
        return {8.2f, 12.0f, 0.6f};
    case Role::OL:
        return {6.4f, 9.0f, 0.65f};
    case Role::DL:
        return {7.0f, 10.0f, 0.65f};
    case Role::LB:
        return {8.2f, 12.0f, 0.6f};
    case Role::CB:
        return {9.0f, 14.0f, 0.5f};
    case Role::S:
        return {8.8f, 13.0f, 0.55f};
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

    int coverTargetId = kNone;  // man-coverage receiver's id
    int receiverNumber = kNone; // 0-based throw button for eligible receivers

    void handle(PlayerEvent event) { state = nextState(state, event, assignment); }
};

} // namespace gridiron
