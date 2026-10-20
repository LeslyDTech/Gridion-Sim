#pragma once

#include "RenderContext.h"

namespace gridiron::render {

// World space (inside BeginMode2D): turf, end zones with team names, lines, hash marks and
// painted yard numbers. Text is drawn in world units too, so players are drawn on top of it.
void drawField(const RenderContext& context);

// Screen space: darkens the edges of the screen (polish only).
void drawVignette(const RenderContext& context);

} // namespace gridiron::render
