#pragma once

#include "Drive.h"
#include "PlayDefinition.h"
#include "World.h"

#include <vector>

namespace gridiron {

// Where the next play starts, given how the last one ended (pure, so it is easy to test).
Vec2 nextBallSpot(const PlayOutcome& outcome, Vec2 previousSpot);

// The sequence of plays: sets up the World, runs the play clock, records the result in the
// drive (down, distance, score) the moment a play ends, waits while the result is shown, then
// spots the ball for the next snap. The selected play stays selected until the user picks another
// (keys 1-4 or Tab before the snap).
class Game {
public:
    static constexpr float kResultSeconds = 2.5f;     // how long the result stays on screen
    static constexpr float kPlayClockSeconds = 40.0f; // NFL play clock after a play ends

    // Precondition: `playbook` is not empty.
    Game(std::vector<PlayDefinition> playbook, PlayDefinition defense);

    void step(const InputCommands& input, float dt);

    const World& world() const { return m_world; }
    const Drive& drive() const { return m_drive; }
    const PlayDefinition& currentPlay() const { return m_playbook[m_playIndex]; }
    const std::vector<PlayDefinition>& playbook() const { return m_playbook; }
    std::size_t currentPlayIndex() const { return m_playIndex; }
    const PlayDefinition& defense() const { return m_defense; }
    int playsRun() const { return m_playsRun; }

    // Whole seconds left on the play clock: counts down before the snap, frozen after it.
    // Presentation of a real rule clock only; running out has no penalty yet.
    int playClockSeconds() const;

    // What the last finished play meant for the drive (e.g. FirstDown). Updated the moment the
    // play ends, so the result can be shown while the play is still on screen.
    DriveEvent lastDriveEvent() const { return m_lastDriveEvent; }

    // Seconds since the current play ended (0 while it is live); drives the result animation.
    float secondsSincePlayEnded() const { return m_resultTimer; }

    // True on the step a new play was set up (lets the camera jump instead of panning).
    bool playJustStarted() const { return m_playJustStarted; }

private:
    void setupPlay();
    void selectPlay(std::size_t index);
    void onPlayEnded();
    void advanceToNextPlay();

    std::vector<PlayDefinition> m_playbook;
    PlayDefinition m_defense;
    std::size_t m_playIndex = 0;
    Vec2 m_ballSpot;
    World m_world;
    Drive m_drive;
    float m_resultTimer = 0.0f;
    float m_playClock = kPlayClockSeconds;
    DriveEvent m_lastDriveEvent = DriveEvent::None;
    int m_playsRun = 0;
    bool m_playJustStarted = true;
};

} // namespace gridiron
