#pragma once

#include "Ball.h"
#include "Player.h"
#include "Vec2.h"

#include <vector>

// Decision-making for AI-controlled players. Each function reads the world and returns a desired
// velocity; the World applies acceleration limits and moves the player. Keeping "decide" separate
// from "move" means every behavior is a small pure function of its inputs.
namespace gridiron::ai {

// Read-only view of the world that behaviors may look at.
struct Context {
    const std::vector<Player>& players;
    const Ball& ball;
    float lineOfScrimmageX;
};

// Desired velocity for a player whose movement the AI decides. Players running a route or
// carrying the ball are moved by the World directly (route math and user input, respectively).
Vec2 desiredVelocity(const Player& self, const Context& context);

} // namespace gridiron::ai
