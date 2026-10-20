#pragma once

#include "RenderContext.h"

namespace gridiron {

class Effects;
class World;

// Stateless drawing functions for everything on the field except the field itself
// (FieldRenderer) and the HUD (Hud). Shapes are drawn in world units (yards) inside the
// camera's 2D mode; labels are drawn afterwards in screen space so they stay crisp.
namespace render {

// --- World space (between BeginMode2D / EndMode2D) ---
void drawLineOfScrimmage(const World& world, const RenderContext& context);
void drawFirstDownLine(float x, const RenderContext& context);
// While a pass is in the air: a pulsing marker where it will land (polish only).
void drawPassTarget(const World& world, const RenderContext& context);
// `opacity` lets the overlay fade out after the snap (see Effects::routeOpacity).
void drawRoutes(const World& world, const RenderContext& context, float opacity);
void drawPlayers(const World& world, const RenderContext& context, const Effects& effects);
void drawBall(const World& world, const RenderContext& context);

// --- Screen space (after EndMode2D) ---
// `debug` (F1) adds every player's state and shows all jersey numbers; with polish on and debug
// off, only skill-position numbers (QB, running backs, receivers, tight ends) are shown.
void drawPlayerLabels(const World& world, const RenderContext& context, bool debug);

} // namespace render
} // namespace gridiron
