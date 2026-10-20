#pragma once

#include "World.h"

#include <cstdint>

namespace gridiron {

// What a finished play meant for the drive.
enum class DriveEvent : std::uint8_t {
    None, // next down
    FirstDown,
    Touchdown,       // +7 (the extra point is assumed)
    Safety,          // +2 for the defense
    TurnoverOnDowns, // failed on 4th down
};

// Down, distance and score. The rules live here, not in the HUD.
//
// Simplification: the user always plays offense. After a score or a turnover on downs, a new
// drive starts at the offense's own 25.
class Drive {
public:
    // `unlimitedDowns` (practice): failing on 4th down starts a new set of downs at the spot
    // instead of turning the ball over.
    explicit Drive(bool unlimitedDowns = false) : m_unlimitedDowns(unlimitedDowns) {
        startSeries(kNewDriveLineX);
    }

    // Applies a finished play. `nextLineOfScrimmageX` is where the ball will be spotted
    // (see nextBallSpot). Returns what happened so Game can reset the field if needed.
    DriveEvent recordPlay(const PlayOutcome& outcome, float nextLineOfScrimmageX);

    int down() const { return m_down; }
    float firstDownLineX() const { return m_firstDownLineX; }
    bool goalToGo() const;
    // Whole yards to go from the given line of scrimmage, at least 1.
    int yardsToGo(float lineOfScrimmageX) const;

    int homeScore() const { return m_homeScore; }
    int awayScore() const { return m_awayScore; }

    static constexpr float kNewDriveLineX = 35.0f; // own 25

private:
    void startSeries(float lineOfScrimmageX);

    int m_down = 1;
    float m_firstDownLineX = 0.0f;
    int m_homeScore = 0;
    int m_awayScore = 0;
    bool m_unlimitedDowns = false;
};

} // namespace gridiron
