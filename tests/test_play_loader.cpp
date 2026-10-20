#include "core/PlayLoader.h"

#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>

#include <string>

using namespace gridiron;
using Catch::Matchers::ContainsSubstring;

namespace {

// Builds an 11-player offensive play whose 11th player is `special`, so each test can inject one
// interesting player without repeating the whole formation.
std::string makeOffense(const std::string& special,
                        const std::string& ballCarrier = R"("ballCarrier": "QB",)") {
    return R"({ "name": "Test Play", "side": "offense", )" + ballCarrier + R"(
      "players": [
        { "slot": "QB", "role": "QB", "jersey": 12, "x": -5, "y": 0, "assignment": "none" },
        { "slot": "WR1", "role": "WR", "jersey": 11, "x": -1, "y": -18, "assignment": "route",
          "route": [[5, 0], [10, 5]] },
        { "slot": "O1", "role": "OL", "jersey": 61, "x": -1, "y": 2, "assignment": "block" },
        { "slot": "O2", "role": "OL", "jersey": 62, "x": -1, "y": 4, "assignment": "block" },
        { "slot": "O3", "role": "OL", "jersey": 63, "x": -1, "y": 6, "assignment": "block" },
        { "slot": "O4", "role": "OL", "jersey": 64, "x": -1, "y": 8, "assignment": "block" },
        { "slot": "O5", "role": "OL", "jersey": 65, "x": -1, "y": 10, "assignment": "block" },
        { "slot": "O6", "role": "OL", "jersey": 66, "x": -1, "y": 12, "assignment": "block" },
        { "slot": "O7", "role": "OL", "jersey": 67, "x": -1, "y": 14, "assignment": "block" },
        { "slot": "O8", "role": "OL", "jersey": 68, "x": -1, "y": 16, "assignment": "block" },
        )" +
           special + R"(
      ] })";
}

const std::string kLineman =
    R"({ "slot": "O9", "role": "OL", "jersey": 69, "x": -1, "y": 18, "assignment": "block" })";

} // namespace

TEST_CASE("parsePlay reads a valid play", "[loader]") {
    const ParseResult<PlayDefinition> result = parsePlay(makeOffense(kLineman));
    REQUIRE(result.ok());

    const PlayDefinition& play = *result.value;
    CHECK(play.name == "Test Play");
    CHECK(play.side == Team::Offense);
    CHECK(play.ballCarrierSlot == "QB");
    REQUIRE(play.players.size() == 11);

    const SlotDefinition& receiver = play.players[1];
    CHECK(receiver.slot == "WR1");
    CHECK(receiver.role == Role::WR);
    CHECK(receiver.assignment == Assignment::RunRoute);
    CHECK(receiver.jersey == 11);
    CHECK(receiver.alignment == Vec2{-1.0f, -18.0f});
    REQUIRE(receiver.route.size() == 2);
    CHECK(receiver.route[1] == Vec2{10.0f, 5.0f});
}

TEST_CASE("parsePlay reports a syntax error", "[loader]") {
    const auto result = parsePlay(R"({ "name": "Broken", )");
    CHECK_FALSE(result.ok());
    CHECK_THAT(result.error, ContainsSubstring("invalid JSON"));
}

TEST_CASE("parsePlay requires exactly 11 players", "[loader]") {
    const auto result =
        parsePlay(R"({ "name": "Short", "side": "offense", "ballCarrier": "QB", "players": [] })");
    CHECK_FALSE(result.ok());
    CHECK_THAT(result.error, ContainsSubstring("exactly 11"));
}

TEST_CASE("parsePlay names the field that is wrong", "[loader]") {
    SECTION("missing field") {
        const auto result = parsePlay(makeOffense(
            R"({ "slot": "O9", "role": "OL", "x": -1, "y": 18, "assignment": "block" })"));
        CHECK_THAT(result.error, ContainsSubstring("players[10].jersey: missing"));
    }
    SECTION("wrong type") {
        const auto result = parsePlay(makeOffense(
            R"({ "slot": "O9", "role": "OL", "jersey": "69", "x": -1, "y": 18, "assignment": "block" })"));
        CHECK_THAT(result.error, ContainsSubstring("players[10].jersey: expected an integer"));
    }
    SECTION("unknown enum value") {
        const auto result = parsePlay(makeOffense(
            R"({ "slot": "O9", "role": "OL", "jersey": 69, "x": -1, "y": 18, "assignment": "dance" })"));
        CHECK_THAT(result.error, ContainsSubstring("unknown value 'dance'"));
    }
    SECTION("route runner without a route") {
        const auto result = parsePlay(makeOffense(
            R"({ "slot": "WR9", "role": "WR", "jersey": 19, "x": -1, "y": 18, "assignment": "route" })"));
        CHECK_THAT(result.error, ContainsSubstring("players[10].route: missing"));
    }
    SECTION("malformed waypoint") {
        const auto result = parsePlay(makeOffense(
            R"({ "slot": "WR9", "role": "WR", "jersey": 19, "x": -1, "y": 18, "assignment": "route",
                 "route": [[5, 0], [5]] })"));
        CHECK_THAT(result.error, ContainsSubstring("players[10].route[1]: expected a point"));
    }
}

TEST_CASE("parsePlay enforces football rules", "[loader]") {
    SECTION("duplicate slot") {
        const auto result = parsePlay(makeOffense(
            R"({ "slot": "O1", "role": "OL", "jersey": 69, "x": -1, "y": 18, "assignment": "block" })"));
        CHECK_THAT(result.error, ContainsSubstring("duplicate slot 'O1'"));
    }
    SECTION("offside") {
        const auto result = parsePlay(makeOffense(
            R"({ "slot": "O9", "role": "OL", "jersey": 69, "x": 2, "y": 18, "assignment": "block" })"));
        CHECK_THAT(result.error, ContainsSubstring("offside"));
    }
    SECTION("overlapping players") {
        // O8 is at (-1, 16); a lineman half a yard away overlaps it (radii 0.65 + 0.65).
        const auto result = parsePlay(makeOffense(
            R"({ "slot": "O9", "role": "OL", "jersey": 69, "x": -1, "y": 16.5, "assignment": "block" })"));
        CHECK_THAT(result.error, ContainsSubstring("'O9' overlaps 'O8'"));
    }
    SECTION("defensive role on offense") {
        const auto result = parsePlay(makeOffense(
            R"({ "slot": "O9", "role": "CB", "jersey": 69, "x": -1, "y": 18, "assignment": "block" })"));
        CHECK_THAT(result.error, ContainsSubstring("does not belong"));
    }
    SECTION("ball carrier must exist") {
        const auto result = parsePlay(makeOffense(kLineman, R"("ballCarrier": "HB",)"));
        CHECK_THAT(result.error, ContainsSubstring("no player with slot 'HB'"));
    }
}

TEST_CASE("All shipped play files load without errors", "[loader][data]") {
    const std::string dataDir = GRIDIRON_DATA_DIR;

    const Playbook offense = loadPlaysFromDirectory(dataDir + "/plays");
    for (const std::string& error : offense.errors) {
        UNSCOPED_INFO(error);
    }
    CHECK(offense.errors.empty());
    CHECK(offense.plays.size() == 4);

    const Playbook defense = loadPlaysFromDirectory(dataDir + "/defense");
    CHECK(defense.errors.empty());
    REQUIRE(defense.plays.size() == 1);
    CHECK(defense.plays[0].side == Team::Defense);
}
