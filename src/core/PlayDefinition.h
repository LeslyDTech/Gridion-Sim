#pragma once

#include "Player.h"
#include "Vec2.h"

#include <string>
#include <vector>

namespace gridiron {

// One player's line in a play file. All positions are relative, so the same play can be run
// from any spot on the field:
//   alignment: offset from the ball. x > 0 is downfield (defense side), x < 0 is the offensive
//              backfield. y > 0 is the offense's right (down on screen).
//   route:     waypoints relative to the player's own alignment.
struct SlotDefinition {
    std::string slot; // unique name within the play, e.g. "WR1", "LT", "CB2"
    Role role = Role::WR;
    Assignment assignment = Assignment::None;
    int jersey = 0;
    Vec2 alignment;
    std::vector<Vec2> route; // RunRoute only
    std::string coverSlot;   // ManCoverage only: the offensive slot to shadow
};

// A play or a defensive call, as loaded from data/plays/*.json or data/defense/*.json.
struct PlayDefinition {
    static constexpr std::size_t kPlayersPerSide = 11;

    std::string name;
    Team side = Team::Offense;
    std::string ballCarrierSlot; // offense only: who holds the ball after the snap
    std::vector<SlotDefinition> players;
};

} // namespace gridiron
