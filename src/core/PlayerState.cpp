#include "PlayerState.h"

namespace gridiron {

PlayerState stateAfterSnap(Assignment assignment) {
    switch (assignment) {
    case Assignment::RunRoute:
        return PlayerState::RunningRoute;
    case Assignment::Block:
        return PlayerState::Blocking;
    case Assignment::ManCoverage:
        return PlayerState::Covering;
    case Assignment::PassRush:
    case Assignment::Pursue:
        return PlayerState::Pursuing;
    case Assignment::None:
        return PlayerState::Idle;
    }
    return PlayerState::Idle;
}

PlayerState nextState(PlayerState current, PlayerEvent event, Assignment assignment) {
    // Reset works from anywhere, so a new play always starts clean.
    if (event == PlayerEvent::Reset) {
        return PlayerState::PreSnap;
    }

    switch (current) {
    case PlayerState::PreSnap:
        if (event == PlayerEvent::Snap) return stateAfterSnap(assignment);
        if (event == PlayerEvent::ReceivedBall) return PlayerState::BallCarrier;
        break;

    case PlayerState::RunningRoute:
        if (event == PlayerEvent::RouteFinished) return PlayerState::Idle;
        if (event == PlayerEvent::TargetedByPass) return PlayerState::ReceivingPass;
        if (event == PlayerEvent::ReceivedBall) return PlayerState::BallCarrier;
        if (event == PlayerEvent::PassCompleted) return PlayerState::Blocking;
        break;

    case PlayerState::Idle:
        if (event == PlayerEvent::TargetedByPass) return PlayerState::ReceivingPass;
        if (event == PlayerEvent::ReceivedBall) return PlayerState::BallCarrier;
        if (event == PlayerEvent::PassCompleted) return PlayerState::Blocking;
        break;

    case PlayerState::ReceivingPass:
        if (event == PlayerEvent::ReceivedBall) return PlayerState::BallCarrier;
        if (event == PlayerEvent::PassCompleted) return PlayerState::Blocking;
        break;

    case PlayerState::Blocking:
        if (event == PlayerEvent::ReceivedBall) return PlayerState::BallCarrier;
        break;

    case PlayerState::BallCarrier:
        if (event == PlayerEvent::ThrewBall) return PlayerState::Idle;
        if (event == PlayerEvent::Tackled) return PlayerState::Tackled;
        break;

    case PlayerState::Covering:
        if (event == PlayerEvent::PassCompleted) return PlayerState::Pursuing;
        break;

    case PlayerState::Pursuing:
    case PlayerState::Tackled:
        break; // only Reset (handled above) leaves these
    }
    return current;
}

const char* toString(PlayerState state) {
    switch (state) {
    case PlayerState::PreSnap:
        return "PreSnap";
    case PlayerState::RunningRoute:
        return "Route";
    case PlayerState::Blocking:
        return "Block";
    case PlayerState::Covering:
        return "Cover";
    case PlayerState::Pursuing:
        return "Pursue";
    case PlayerState::BallCarrier:
        return "Carrier";
    case PlayerState::ReceivingPass:
        return "Catch";
    case PlayerState::Idle:
        return "Idle";
    case PlayerState::Tackled:
        return "Tackled";
    }
    return "?";
}

} // namespace gridiron
