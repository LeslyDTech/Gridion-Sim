#pragma once

#include "RenderContext.h"

namespace gridiron {

class Game;

namespace render {

// Screen-space game information: play call, down & distance, score, play clock, controls help
// and (plain mode) the result banner. With polish on, a broadcast-style scorebug is drawn.
void drawHud(const Game& game, const RenderContext& context);

// Pixels at the bottom of the screen the HUD covers in the current mode (for the camera).
int hudBottomInset(bool polish);

} // namespace render
} // namespace gridiron
