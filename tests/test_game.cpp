// Game-level state the HUD reads: play clock, play selection and the drive update at play end.
#include "core/Game.h"
#include "core/PlayLoader.h"

#include <catch2/catch_test_macros.hpp>

#include <string>

using namespace gridiron;

namespace {

constexpr float kDt = 1.0f / 60.0f;

Game makeGame() {
    const std::string dataDir = GRIDIRON_DATA_DIR;
    Playbook offense = loadPlaysFromDirectory(dataDir + "/plays");
    Playbook defense = loadPlaysFromDirectory(dataDir + "/defense");
    REQUIRE(offense.plays.size() == 4);
    REQUIRE(defense.plays.size() == 1);
    return Game(std::move(offense.plays), std::move(defense.plays.front()));
}

// Whole steps, not a float accumulator, so the step count is exact.
void runSeconds(Game& game, const InputCommands& input, float seconds) {
    const int steps = static_cast<int>(seconds * 60.0f + 0.5f);
    for (int i = 0; i < steps; ++i) {
        game.step(input, kDt);
    }
}

InputCommands pressSnap() {
    InputCommands input;
    input.snap = true;
    return input;
}

} // namespace

TEST_CASE("Play clock runs before the snap and freezes after it", "[game]") {
    Game game = makeGame();
    CHECK(game.playClockSeconds() == 40);

    runSeconds(game, InputCommands{}, 5.5f); // 34.5 s left, shown as 35 (rounded up, like TV)
    CHECK(game.playClockSeconds() == 35);

    game.step(pressSnap(), kDt);
    const int atSnap = game.playClockSeconds();
    runSeconds(game, InputCommands{}, 1.0f);
    CHECK(game.playClockSeconds() == atSnap);
}

TEST_CASE("Number keys select a play only before the snap", "[game]") {
    Game game = makeGame();
    CHECK(game.currentPlayIndex() == 0);

    InputCommands select;
    select.selectPlay = 2;
    game.step(select, kDt);
    CHECK(game.currentPlayIndex() == 2);
    CHECK(game.currentPlay().name == "HB Dive");

    game.step(pressSnap(), kDt);
    select.selectPlay = 0;
    game.step(select, kDt);
    CHECK(game.currentPlayIndex() == 2); // ignored while the play is live
}

TEST_CASE("Play selection: a number key chooses, Tab browses, a new play reopens it", "[game]") {
    Game game = makeGame();
    CHECK(game.selectingPlay());

    InputCommands select;
    select.selectPlay = 0; // the play that is already highlighted still counts as a choice
    game.step(select, kDt);
    CHECK_FALSE(game.selectingPlay());

    InputCommands tab;
    tab.nextPlay = true;
    game.step(tab, kDt);
    CHECK(game.selectingPlay());
    CHECK(game.currentPlayIndex() == 1);

    game.step(pressSnap(), kDt);
    CHECK_FALSE(game.selectingPlay()); // live play
}

TEST_CASE("Restart drive resets downs, score, spot and play selection", "[game]") {
    Game game = makeGame();
    InputCommands select;
    select.selectPlay = 2;
    game.step(select, kDt);
    game.step(pressSnap(), kDt);
    for (int i = 0; i < 60 * 20 && game.world().phase() != PlayPhase::Over; ++i) {
        game.step(InputCommands{}, kDt);
    }
    REQUIRE(game.drive().down() == 2);

    game.restartDrive();
    CHECK(game.drive().down() == 1);
    CHECK(game.lineOfScrimmageX() == Drive::kNewDriveLineX);
    CHECK(game.world().phase() == PlayPhase::PreSnap);
    CHECK(game.selectingPlay());
    CHECK(game.playClockSeconds() == 40);
}

TEST_CASE("Out-of-range play selections are ignored", "[game]") {
    Game game = makeGame();
    InputCommands select;
    select.selectPlay = 9;
    game.step(select, kDt);
    CHECK(game.currentPlayIndex() == 0);
}

TEST_CASE("The drive updates the moment a play ends and the play stays selected", "[game]") {
    Game game = makeGame();
    InputCommands select;
    select.selectPlay = 2; // HB Dive: standing still behind the line ends in a tackle for a loss
    game.step(select, kDt);
    game.step(pressSnap(), kDt);

    for (int i = 0; i < 60 * 20 && game.world().phase() != PlayPhase::Over; ++i) {
        game.step(InputCommands{}, kDt);
    }
    REQUIRE(game.world().phase() == PlayPhase::Over);
    CHECK(game.drive().down() == 2); // already 2nd down while the result is shown
    CHECK(game.lastDriveEvent() == DriveEvent::None);
    // Down & distance is measured from the new spot, not the old line of scrimmage.
    const float newLine = game.lineOfScrimmageX();
    CHECK(newLine < game.world().lineOfScrimmageX()); // a loss moved the ball back
    CHECK(game.drive().yardsToGo(newLine) > 10);
    CHECK(game.secondsSincePlayEnded() >= 0.0f);

    runSeconds(game, InputCommands{}, Game::kResultSeconds + 0.1f);
    CHECK(game.world().phase() == PlayPhase::PreSnap);
    CHECK(game.currentPlayIndex() == 2);
    CHECK(game.playClockSeconds() == 40); // reset for the new play
    CHECK(game.selectingPlay());          // and the playbook is open again
}
