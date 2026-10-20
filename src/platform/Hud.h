#pragma once

#include "FollowCamera.h"
#include "RenderContext.h"

namespace gridiron {

class Game;

namespace render::broadcast {
class HudAnimator;
}

namespace render {

// Screen-space game information: play call, down & distance, score, play clock, controls help
// and (plain mode) the result banner. With polish on, a broadcast-style scorebug is drawn.
void drawHud(const Game& game, const RenderContext& context,
             const broadcast::HudAnimator& animator);

// The screen margins the HUD covers in the current mode, plus a small gap: the camera keeps every
// player out of them.
SafeArea hudSafeArea(bool polish);

} // namespace render
} // namespace gridiron
