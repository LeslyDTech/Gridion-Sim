#include "core/Player.h"
#include "core/PlayerState.h"

#include <catch2/catch_test_macros.hpp>

using namespace gridiron;

TEST_CASE("Snap sends each assignment to its job", "[state]") {
    CHECK(nextState(PlayerState::PreSnap, PlayerEvent::Snap, Assignment::RunRoute) ==
          PlayerState::RunningRoute);
    CHECK(nextState(PlayerState::PreSnap, PlayerEvent::Snap, Assignment::Block) ==
          PlayerState::Blocking);
    CHECK(nextState(PlayerState::PreSnap, PlayerEvent::Snap, Assignment::ManCoverage) ==
          PlayerState::Covering);
    CHECK(nextState(PlayerState::PreSnap, PlayerEvent::Snap, Assignment::PassRush) ==
          PlayerState::Pursuing);
    CHECK(nextState(PlayerState::PreSnap, PlayerEvent::Snap, Assignment::Pursue) ==
          PlayerState::Pursuing);
    CHECK(nextState(PlayerState::PreSnap, PlayerEvent::Snap, Assignment::None) ==
          PlayerState::Idle);
}

TEST_CASE("A full pass play walks a receiver through its states", "[state]") {
    Player receiver;
    receiver.assignment = Assignment::RunRoute;

    receiver.handle(PlayerEvent::Snap);
    CHECK(receiver.state == PlayerState::RunningRoute);
    receiver.handle(PlayerEvent::TargetedByPass);
    CHECK(receiver.state == PlayerState::ReceivingPass);
    receiver.handle(PlayerEvent::ReceivedBall);
    CHECK(receiver.state == PlayerState::BallCarrier);
    receiver.handle(PlayerEvent::Tackled);
    CHECK(receiver.state == PlayerState::Tackled);
}

TEST_CASE("Quarterback becomes idle after throwing", "[state]") {
    Player qb;
    qb.handle(PlayerEvent::ReceivedBall);
    CHECK(qb.state == PlayerState::BallCarrier);
    qb.handle(PlayerEvent::ThrewBall);
    CHECK(qb.state == PlayerState::Idle);
}

TEST_CASE("A completed pass turns coverage into pursuit and routes into blocks", "[state]") {
    CHECK(nextState(PlayerState::Covering, PlayerEvent::PassCompleted, Assignment::ManCoverage) ==
          PlayerState::Pursuing);
    CHECK(nextState(PlayerState::RunningRoute, PlayerEvent::PassCompleted, Assignment::RunRoute) ==
          PlayerState::Blocking);
}

TEST_CASE("Events that don't apply leave the state unchanged", "[state]") {
    CHECK(nextState(PlayerState::PreSnap, PlayerEvent::Tackled, Assignment::RunRoute) ==
          PlayerState::PreSnap);
    CHECK(nextState(PlayerState::Pursuing, PlayerEvent::RouteFinished, Assignment::Pursue) ==
          PlayerState::Pursuing);
    CHECK(nextState(PlayerState::Blocking, PlayerEvent::Snap, Assignment::Block) ==
          PlayerState::Blocking);
}

TEST_CASE("Tackled is terminal until Reset", "[state]") {
    for (const PlayerEvent event :
         {PlayerEvent::Snap, PlayerEvent::ReceivedBall, PlayerEvent::ThrewBall,
          PlayerEvent::RouteFinished, PlayerEvent::TargetedByPass, PlayerEvent::PassCompleted,
          PlayerEvent::Tackled}) {
        CHECK(nextState(PlayerState::Tackled, event, Assignment::RunRoute) == PlayerState::Tackled);
    }
    CHECK(nextState(PlayerState::Tackled, PlayerEvent::Reset, Assignment::RunRoute) ==
          PlayerState::PreSnap);
}
