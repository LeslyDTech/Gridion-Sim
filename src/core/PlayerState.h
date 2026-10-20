#pragma once

#include <cstdint>

namespace gridiron {

// What a player is doing right now. Each player's behavior (PlayerAi) is chosen by this value.
//
// A plain enum plus one transition function (nextState) instead of a class per state: every legal
// transition is visible in one switch statement, players stay plain copyable values, and the
// whole machine can be tested without constructing a world.
enum class PlayerState : std::uint8_t {
    PreSnap,       // lined up, waiting for the snap
    RunningRoute,  // receiver following route waypoints
    Blocking,      // offensive player shielding the ball carrier from a defender
    Covering,      // defender shadowing an assigned receiver (man coverage)
    Pursuing,      // defender chasing the ball carrier (or the ball in the air)
    BallCarrier,   // has the ball; the user controls this player
    ReceivingPass, // targeted receiver running to where the ball will arrive
    Idle,          // assignment finished (QB after throwing, receiver after route)
    Tackled,       // ball carrier brought down; only Reset leaves this state
};

// Something that happened to a player. The World raises these; nextState decides what they mean.
enum class PlayerEvent : std::uint8_t {
    Snap,           // ball snapped: switch from PreSnap to the assigned job
    ReceivedBall,   // got the ball (snap, handoff or catch)
    ThrewBall,      // ball carrier released a pass
    RouteFinished,  // reached the last waypoint
    TargetedByPass, // the user threw to this receiver
    PassCompleted,  // someone else caught a pass: offense blocks, defense pursues
    Tackled,        // brought down while carrying the ball
    Reset,          // new play: back to PreSnap
};

// The player's job for this play, read from the play's JSON file.
enum class Assignment : std::uint8_t {
    None,        // no job after the snap (e.g. QB on a handoff)
    RunRoute,    // run the route waypoints
    Block,       // protect the ball carrier
    PassRush,    // attack the ball carrier immediately
    ManCoverage, // shadow one receiver until the ball is caught
    Pursue,      // read the play and chase the ball (zone/spy, simplified)
};

// The whole state machine. Events that don't apply to the current state are ignored and the
// current state is returned unchanged, so callers can broadcast events safely.
PlayerState nextState(PlayerState current, PlayerEvent event, Assignment assignment);

// The state a player enters at the snap, based on their assignment.
PlayerState stateAfterSnap(Assignment assignment);

const char* toString(PlayerState state);

} // namespace gridiron
