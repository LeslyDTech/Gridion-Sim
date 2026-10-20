#pragma once

namespace gridiron {

// One screen of the application: loading, title, menus, the game, pause...
//
// Scenes live on a SceneStack. Only the top scene is updated, so a scene pushed on top (Pause)
// freezes everything below it. Drawing starts at the highest non-overlay scene and goes up, so an
// overlay (Pause) is drawn over a frozen frame of the scene beneath it.
//
// This header is raylib-free on purpose: the stack logic is unit tested without a window.
class Scene {
public:
    virtual ~Scene() = default;

    // Called when the scene becomes part of the stack (pushed, or replaced in).
    virtual void enter() {}
    // Called when the scene leaves the stack (popped, replaced, or reset away).
    virtual void exit() {}

    virtual void update(float frameSeconds) = 0;
    virtual void draw() = 0;

    // True if the scene below should be drawn first, with this one on top.
    virtual bool isOverlay() const { return false; }
};

} // namespace gridiron
