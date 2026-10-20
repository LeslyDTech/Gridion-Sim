#pragma once

#include "AppContext.h"
#include "MenuWidgets.h"
#include "app/Scene.h"

namespace gridiron {

class GameScene;

// Pause (Figma 8:423): pushed on top of GameScene with Esc / Start. It's an overlay, so the frozen
// game frame is drawn first; the simulation doesn't run because only the top scene updates. The
// darkening overlay and vignette dim the game, and the live scorebug is drawn again on top so it
// stays fully visible, as in the design.
class PauseScene : public Scene {
public:
    PauseScene(AppContext& context, GameScene& game);

    void update(float frameSeconds) override;
    void draw() override;
    bool isOverlay() const override { return true; }

private:
    menu::ListLayout optionsLayout() const;
    void activate(int option);

    AppContext& m_context;
    GameScene& m_game;
    menu::Selection m_selection;
};

} // namespace gridiron
