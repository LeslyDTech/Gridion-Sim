#pragma once

#include "core/Player.h"
#include "core/Vec2.h"

#include <raylib.h>

namespace gridiron {

class World;

// Picks a player's tile in the Figma player atlas (assets/sprites/players.png, layout in
// theme::sprite). The atlas has every combination of team x position x pose x facing direction.
namespace sprite {

// Atlas order within one team. Two roles share each of RB (HB, FB) and DB (CB, S).
enum class Position { QB, RB, WR, TE, OL, DL, LB, DB, K };
enum class Pose { Idle, Run, Block, Tackle, Catch, Throw, Fall, Kick };
enum class Direction { Down, Up, Left, Right };

Position positionFor(Role role);

// What the player is doing this frame, from simulation state only (no animation clock):
//   Fall    tackled
//   Throw   the QB for a moment after releasing a pass
//   Catch   the target receiver as the ball arrives
//   Block   blockers, and defenders held in a block
//   Tackle  defenders closing on the ball carrier
//   Run     anyone moving faster than a walk
//   Idle    everyone else (and everyone before the snap)
// `strideIntensity` is Effects::Stride::intensity: 0 standing, 1 at top speed.
Pose poseFor(const Player& player, const World& world, float strideIntensity);

// The four directions are exact 90-degree rotations of each other, so the nearest one is drawn,
// rotated by what is left over (within +-45 degrees). Turning is smooth, and the art (and the
// number on the jersey) is never more than 45 degrees from how it was drawn.
struct Facing {
    Direction direction;
    float residualDegrees; // clockwise on screen
};
Facing facingFor(Vec2 facing);

// Source rectangle of one tile in the atlas.
Rectangle tile(Team team, Position position, Pose pose, Direction direction);

} // namespace sprite
} // namespace gridiron
