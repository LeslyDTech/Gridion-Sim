#pragma once

#include "Effects.h"
#include "FollowCamera.h"
#include "core/FixedTimestep.h"
#include "core/Game.h"
#include "core/InputCommands.h"

#include <vector>

namespace gridiron {

class Assets;

// The live football behind the title screen and main menu: the real simulation and AI, run by a
// small autopilot instead of a player. It cycles through the playbook, snaps, throws to the most
// open receiver (or hands off), and runs the ball carrier away from the nearest defender, while
// the camera drifts slowly over the action. Nothing in the core knows it isn't a real game: the
// autopilot only produces the same InputCommands a player would.
//
// Lives across Title and Main Menu (owned by the app context) so the play doesn't restart when
// you move between them.
class AttractMode {
public:
    AttractMode(std::vector<PlayDefinition> playbook, PlayDefinition defense);

    void update(float frameSeconds);
    void draw(const Assets& assets);

private:
    InputCommands autopilot(float dt);
    Vec2 runDirection(const Player& carrier) const;
    int mostOpenReceiver() const;

    Game m_game;
    FixedTimestep m_timestep;
    FollowCamera m_camera;
    Effects m_effects;
    float m_alpha = 0.0f;
    float m_phaseSeconds = 0.0f; // time in the current play phase (autopilot timing)
    PlayPhase m_lastPhase = PlayPhase::PreSnap;
    std::size_t m_nextPlay = 0;
    double m_time = 0.0;
};

} // namespace gridiron
