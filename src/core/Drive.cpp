#include "Drive.h"

#include "FieldGeometry.h"

#include <algorithm>
#include <cmath>

namespace gridiron {
namespace {

constexpr float kYardsForFirstDown = 10.0f;
constexpr int kMaxDowns = 4;
constexpr int kTouchdownPoints = 7; // touchdown + assumed extra point
constexpr int kSafetyPoints = 2;

} // namespace

static_assert(Drive::kNewDriveLineX == field::kStartingLineOfScrimmageX,
              "a new drive must start where Game spots the ball after a score");

void Drive::startSeries(float lineOfScrimmageX) {
    m_down = 1;
    // Inside the opponent's 10 the line to gain is the goal line ("& Goal").
    m_firstDownLineX = std::min(lineOfScrimmageX + kYardsForFirstDown, field::kOpponentGoalLine);
}

DriveEvent Drive::recordPlay(const PlayOutcome& outcome, float nextLineOfScrimmageX) {
    switch (outcome.result) {
    case PlayResult::Touchdown:
        m_homeScore += kTouchdownPoints;
        startSeries(kNewDriveLineX);
        return DriveEvent::Touchdown;
    case PlayResult::Safety:
        m_awayScore += kSafetyPoints;
        startSeries(kNewDriveLineX);
        return DriveEvent::Safety;
    case PlayResult::Tackled:
    case PlayResult::OutOfBounds:
    case PlayResult::Incomplete:
    case PlayResult::None:
        break;
    }

    if (nextLineOfScrimmageX >= m_firstDownLineX) {
        startSeries(nextLineOfScrimmageX);
        return DriveEvent::FirstDown;
    }
    if (m_down == kMaxDowns) {
        startSeries(kNewDriveLineX);
        return DriveEvent::TurnoverOnDowns;
    }
    ++m_down;
    return DriveEvent::None;
}

bool Drive::goalToGo() const { return m_firstDownLineX >= field::kOpponentGoalLine; }

int Drive::yardsToGo(float lineOfScrimmageX) const {
    return std::max(1, static_cast<int>(std::ceil(m_firstDownLineX - lineOfScrimmageX - 0.01f)));
}

} // namespace gridiron
