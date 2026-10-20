#pragma once

#include "RenderContext.h"

namespace gridiron {

class Game;

namespace render {

// Screen-space game information: play call, controls help and the result of the last play.
void drawHud(const Game& game, const RenderContext& context);

} // namespace render
} // namespace gridiron
