#pragma once

#include "AppContext.h"
#include "BroadcastHud.h"
#include "Effects.h"
#include "FollowCamera.h"
#include "app/Scene.h"
#include "core/FixedTimestep.h"
#include "core/Game.h"

namespace gridiron {

// A drive (or practice session) being played. Owns the core Game and everything needed to drive
// and draw it: fixed timestep, camera, effects, HUD animation. The core Game knows nothing about
// scenes; this class only feeds it input and draws its state.
//
// While Pause is on top of it, update() isn't called: the simulation stops, and draw() keeps
// showing the last frame (camera, alpha and effects are all updated in update(), not draw()).
class GameScene : public Scene {
public:
    GameScene(AppContext& context, GameMode mode);

    void update(float frameSeconds) override;
    void draw() override;

    Game& game() { return m_game; }

private:
    void handleCameraZoom();
    void runSimulationSteps(int steps);
    void updateCamera(float frameSeconds);

    AppContext& m_context;
    Game m_game;
    FixedTimestep m_timestep;
    FollowCamera m_camera;
    Effects m_effects;
    render::broadcast::HudAnimator m_hudAnimator;
    float m_alpha = 0.0f; // interpolation factor of the last update, reused while frozen
};

} // namespace gridiron
