#pragma once

#include "RenderContext.h"

namespace gridiron {

class DebugOverlay;
class Effects;
class Game;
class World;

// Stateless drawing functions for everything on the field except the field itself
// (FieldRenderer) and the HUD (Hud). Shapes are drawn in world units (yards) inside the
// camera's 2D mode; labels are drawn afterwards in screen space so they stay crisp.
namespace render {

// Everything on the field, in order: turf, lines, routes, pass target, particles, players, ball.
// Wraps its own BeginMode2D/EndMode2D. Shared by the game and the attract mode behind the menus.
// `overlay` (may be null) adds the F1 collision grid.
void drawWorldLayers(const Game& game, const RenderContext& context, const Effects& effects,
                     const DebugOverlay* overlay);

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
