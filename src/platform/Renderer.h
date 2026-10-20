#pragma once

namespace gridiron {

class FollowCamera;
class Game;
class World;

// Stateless drawing functions. Shapes are drawn in world units (yards) inside the camera's 2D
// mode; text is drawn afterwards in screen space so it stays crisp at any zoom.
//
// `alpha` is the fixed-timestep interpolation factor: everything is drawn at
// lerp(previousPosition, position, alpha).
namespace render {

// --- World space (between BeginMode2D / EndMode2D) ---
void drawFieldSurface();
void drawLineOfScrimmage(const World& world);
void drawRoutes(const World& world); // pre-snap route preview
void drawPlayers(const World& world, float alpha);
void drawBall(const World& world, float alpha);

// --- Screen space (after EndMode2D) ---
void drawFieldNumbers(const FollowCamera& camera);
void drawPlayerLabels(const World& world, float alpha, const FollowCamera& camera, bool showStates);
void drawHud(const Game& game);

} // namespace render
} // namespace gridiron
