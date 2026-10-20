#include "core/Drive.h"
#include "core/FieldGeometry.h"

#include <catch2/catch_test_macros.hpp>

using namespace gridiron;

namespace {

PlayOutcome tackledAt(float x) { return {PlayResult::Tackled, 0.0f, {x, field::kCenterY}}; }
PlayOutcome incomplete() { return {PlayResult::Incomplete, 0.0f, {}}; }

constexpr float kStart = Drive::kNewDriveLineX;

} // namespace

TEST_CASE("A new drive is 1st & 10 at the own 25 with no score", "[drive]") {
    const Drive drive;
    CHECK(drive.down() == 1);
    CHECK(drive.yardsToGo(kStart) == 10);
    CHECK(drive.homeScore() == 0);
    CHECK(drive.awayScore() == 0);
}

TEST_CASE("Short gains advance the down and shrink the distance", "[drive]") {
    Drive drive;
    CHECK(drive.recordPlay(tackledAt(kStart + 4.0f), kStart + 4.0f) == DriveEvent::None);
    CHECK(drive.down() == 2);
    CHECK(drive.yardsToGo(kStart + 4.0f) == 6);

    CHECK(drive.recordPlay(incomplete(), kStart + 4.0f) == DriveEvent::None);
    CHECK(drive.down() == 3);
}

TEST_CASE("Reaching the line to gain is a first down", "[drive]") {
    Drive drive;
    drive.recordPlay(tackledAt(kStart + 3.0f), kStart + 3.0f);
    CHECK(drive.recordPlay(tackledAt(kStart + 12.0f), kStart + 12.0f) == DriveEvent::FirstDown);
    CHECK(drive.down() == 1);
    CHECK(drive.yardsToGo(kStart + 12.0f) == 10);
}

TEST_CASE("Failing on 4th down turns the ball over and starts a new drive", "[drive]") {
    Drive drive;
    for (int i = 0; i < 3; ++i) {
        drive.recordPlay(incomplete(), kStart);
    }
    CHECK(drive.down() == 4);
    CHECK(drive.recordPlay(incomplete(), kStart) == DriveEvent::TurnoverOnDowns);
    CHECK(drive.down() == 1);
}

TEST_CASE("Inside the 10 the line to gain is the goal line", "[drive]") {
    Drive drive;
    const float atThe6 = field::kOpponentGoalLine - 6.0f;
    drive.recordPlay(tackledAt(atThe6), atThe6);
    CHECK(drive.goalToGo());
    CHECK(drive.yardsToGo(atThe6) == 6);
}

TEST_CASE("Touchdowns and safeties score and restart the drive", "[drive]") {
    Drive drive;
    CHECK(drive.recordPlay({PlayResult::Touchdown, 75.0f, {110.0f, 20.0f}}, kStart) ==
          DriveEvent::Touchdown);
    CHECK(drive.homeScore() == 7);
    CHECK(drive.down() == 1);

    CHECK(drive.recordPlay({PlayResult::Safety, -30.0f, {5.0f, 20.0f}}, kStart) ==
          DriveEvent::Safety);
    CHECK(drive.awayScore() == 2);
}
