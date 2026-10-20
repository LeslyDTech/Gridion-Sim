// Headless end-to-end tests: real play files, real World, no window. These show the core
// simulation runs (and is deterministic) without raylib.
#include "core/FieldGeometry.h"
#include "core/Game.h"
#include "core/PlayLoader.h"
#include "core/World.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace gridiron;

namespace {

constexpr float kDt = 1.0f / 60.0f;

PlayDefinition loadData(const std::string& relativePath) {
    const auto result = loadPlayFile(std::string(GRIDIRON_DATA_DIR) + "/" + relativePath);
    REQUIRE(result.ok());
    return *result.value;
}

// Snaps, then runs straight downfield until the play ends (or a 20-second safety limit).
World runDive(int maxSteps = 60 * 20) {
    World world;
    world.setup(loadData("plays/03_hb_dive.json"), loadData("defense/base_cover1.json"),
                {field::kStartingLineOfScrimmageX, field::kCenterY});
    InputCommands snap;
    snap.snap = true;
    world.step(snap, kDt);

    InputCommands run;
    run.move = {1.0f, 0.0f};
    for (int i = 0; i < maxSteps && world.phase() == PlayPhase::Live; ++i) {
        world.step(run, kDt);
    }
    return world;
}

} // namespace

TEST_CASE("Setup lines up 22 players and spots the ball", "[world]") {
    World world;
    world.setup(loadData("plays/01_slant.json"), loadData("defense/base_cover1.json"),
                {35.0f, 26.0f});

    CHECK(world.players().size() == 22);
    CHECK(world.phase() == PlayPhase::PreSnap);
    CHECK(world.ball().state == BallState::Spotted);
    for (const Player& player : world.players()) {
        CHECK(player.state == PlayerState::PreSnap);
        // Offense behind the line, defense in front of it.
        CHECK((player.team == Team::Offense) == (player.position.x < 35.0f));
    }
}

TEST_CASE("A run up the middle ends in a tackle with yards counted from the line", "[world]") {
    const World world = runDive();
    REQUIRE(world.phase() == PlayPhase::Over);
    CHECK(world.outcome().result == PlayResult::Tackled);
    CHECK(world.outcome().yardsGained == world.outcome().spot.x - field::kStartingLineOfScrimmageX);
}

TEST_CASE("The simulation is deterministic", "[world]") {
    const World first = runDive();
    const World second = runDive();
    CHECK(first.outcome().spot == second.outcome().spot);
    CHECK(first.outcome().result == second.outcome().result);
}

TEST_CASE("Running out of bounds ends the play at the sideline", "[world]") {
    World world;
    world.setup(loadData("plays/03_hb_dive.json"), loadData("defense/base_cover1.json"),
                {field::kStartingLineOfScrimmageX, field::kCenterY});
    InputCommands snap;
    snap.snap = true;
    world.step(snap, kDt);

    // Sprint backward and to the sideline, away from the defense.
    InputCommands run;
    run.move = normalized(Vec2{-0.3f, -1.0f});
    for (int i = 0; i < 60 * 10 && world.phase() == PlayPhase::Live; ++i) {
        world.step(run, kDt);
    }
    REQUIRE(world.phase() == PlayPhase::Over);
    if (world.outcome().result == PlayResult::OutOfBounds) {
        CHECK(world.outcome().spot.y == 0.0f);
    } else {
        CHECK(world.outcome().result == PlayResult::Tackled); // caught from behind is fine too
    }
}

TEST_CASE("Blocks hold longer for strong blockers and shorter for good pass rushers", "[world]") {
    const float lineVsLine = blockHoldSeconds(attributesFor(Role::OL), attributesFor(Role::DL));
    CHECK(lineVsLine > 1.5f);
    CHECK(lineVsLine < 2.0f);
    // A receiver blocking a linebacker can't hold him as long as a lineman holds a lineman.
    CHECK(blockHoldSeconds(attributesFor(Role::WR), attributesFor(Role::LB)) < lineVsLine);
}

TEST_CASE("Pass protection gives the quarterback time in the pocket", "[world]") {
    // Shotgun slant, quarterback standing still: before blocking engagement he was sacked in
    // about 1.2 seconds. Five linemen against a four-man rush should hold for 2.5+ seconds.
    World world;
    world.setup(loadData("plays/01_slant.json"), loadData("defense/base_cover1.json"),
                {field::kStartingLineOfScrimmageX, field::kCenterY});
    InputCommands snap;
    snap.snap = true;
    world.step(snap, kDt);
    int steps = 0;
    while (world.phase() == PlayPhase::Live && steps < 60 * 10) {
        world.step(InputCommands{}, kDt);
        ++steps;
    }
    CHECK(static_cast<float>(steps) * kDt >= 2.5f);
}

TEST_CASE("Tackle chance: head-on beats from behind, speed makes a carrier harder to stop",
          "[world]") {
    const PlayerAttributes linebacker = attributesFor(Role::LB);
    const Vec2 running{8.0f, 0.0f}; // carrier running downfield at 8 yd/s
    const Vec2 standing{};

    // Defender in front of the carrier (head-on) vs directly behind him.
    const float headOn = tackleChance(linebacker, standing, running, {1.0f, 0.0f});
    const float fromBehind = tackleChance(linebacker, standing, running, {-1.0f, 0.0f});
    CHECK(headOn > fromBehind);

    // Same angle, carrier at full speed vs jogging.
    const float slowCarrier = tackleChance(linebacker, standing, {3.0f, 0.0f}, {1.0f, 0.0f});
    CHECK(slowCarrier > headOn);

    // Driving into the carrier helps.
    const float closing = tackleChance(linebacker, {-6.0f, 0.0f}, running, {1.0f, 0.0f});
    CHECK(closing > headOn);
}

TEST_CASE("Tackle chance never reaches certainty or zero", "[world]") {
    PlayerAttributes great = attributesFor(Role::LB);
    great.tackling = 1.0f;
    PlayerAttributes poor = attributesFor(Role::QB);
    poor.tackling = 0.0f;
    CHECK(tackleChance(great, {-10.0f, 0.0f}, {}, {1.0f, 0.0f}) <= 0.95f);
    CHECK(tackleChance(poor, {}, {10.0f, 0.0f}, {-1.0f, 0.0f}) >= 0.15f);
}

TEST_CASE("nextBallSpot moves the ball to the tackle, between the hashes", "[game]") {
    const Vec2 start{35.0f, field::kCenterY};

    PlayOutcome tackle{PlayResult::Tackled, 7.0f, {42.0f, 2.0f}};
    const Vec2 spot = nextBallSpot(tackle, start);
    CHECK(spot.x == 42.0f);
    CHECK(spot.y == field::kNearHashY); // tackled near the sideline: spotted on the hash

    PlayOutcome incomplete{PlayResult::Incomplete, 0.0f, {35.0f, 40.0f}};
    CHECK(nextBallSpot(incomplete, start) == start);

    PlayOutcome touchdown{PlayResult::Touchdown, 75.0f, {110.0f, 20.0f}};
    CHECK(nextBallSpot(touchdown, start).x == field::kStartingLineOfScrimmageX);
}
