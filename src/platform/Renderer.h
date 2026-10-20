#pragma once

#include "RenderContext.h"

namespace gridiron {

class World;

// Stateless drawing functions for everything on the field except the field itself
// (FieldRenderer) and the HUD (Hud). Shapes are drawn in world units (yards) inside the
// camera's 2D mode; labels are drawn afterwards in screen space so they stay crisp.
namespace render {

// --- World space (between BeginMode2D / EndMode2D) ---
void drawLineOfScrimmage(const World& world, const RenderContext& context);
void drawRoutes(const World& world, const RenderContext& context);
void drawPlayers(const World& world, const RenderContext& context);
void drawBall(const World& world, const RenderContext& context);

// --- Screen space (after EndMode2D) ---
void drawPlayerLabels(const World& world, const RenderContext& context, bool showStates);

} // namespace render
} // namespace gridiron
