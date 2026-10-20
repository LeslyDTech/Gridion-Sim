#pragma once

namespace gridiron {

class Assets;
class Game;
class World;

// The broadcast HUD from the Figma frame "Football Broadcast HUD / 1280x720" (node 2:286), drawn
// with raylib. One function per Figma group; every size, color and offset comes from
// theme::hud in Theme.h. Each group is anchored to a screen edge so the layout holds at any
// window size:
//   PlayCallBanner   top center        PlaySelectPanel  top right (pre-snap only)
//   YardsGainedPopup screen center     Scorebug         bottom center
//   ControlsBar      bottom, full width
//
// All values shown come from core game state (Game, Drive, World); nothing is hardcoded.
namespace render::broadcast {

void drawPlayCallBanner(const Game& game, const Assets& assets);
void drawPlaySelectPanel(const Game& game, const Assets& assets);
void drawYardsGainedPopup(const Game& game, const Assets& assets);
void drawScorebug(const Game& game, const Assets& assets);
void drawControlsBar(const World& world, const Assets& assets);

// Pixels at the top of the screen covered by the banner, and at the bottom by the scorebug and
// controls bar.
int topInset();
int bottomInset();

} // namespace render::broadcast
} // namespace gridiron
