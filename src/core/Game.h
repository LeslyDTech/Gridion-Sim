#pragma once

#include "PlayDefinition.h"
#include "World.h"

#include <vector>

namespace gridiron {

// Where the next play starts, given how the last one ended (pure, so it is easy to test).
Vec2 nextBallSpot(const PlayOutcome& outcome, Vec2 previousSpot);

// The sequence of plays: sets up the World, waits while the result is shown, spots the ball and
// moves on to the next play in the playbook.
class Game {
public:
    static constexpr float kResultSeconds = 2.5f; // how long the result banner stays up

    // Precondition: `playbook` is not empty.
    Game(std::vector<PlayDefinition> playbook, PlayDefinition defense);

    void step(const InputCommands& input, float dt);

    const World& world() const { return m_world; }
    const PlayDefinition& currentPlay() const { return m_playbook[m_playIndex]; }
    const PlayDefinition& defense() const { return m_defense; }
    int playsRun() const { return m_playsRun; }

    // True on the step a new play was set up (lets the camera jump instead of panning).
    bool playJustStarted() const { return m_playJustStarted; }

private:
    void setupPlay();
    void advanceToNextPlay();

    std::vector<PlayDefinition> m_playbook;
    PlayDefinition m_defense;
    std::size_t m_playIndex = 0;
    Vec2 m_ballSpot;
    World m_world;
    float m_resultTimer = 0.0f;
    int m_playsRun = 0;
    bool m_playJustStarted = true;
};

} // namespace gridiron
