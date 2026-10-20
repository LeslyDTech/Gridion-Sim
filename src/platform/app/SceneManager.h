#pragma once

#include "SceneStack.h"

#include <memory>

namespace gridiron {

// Runs the scene stack with fade transitions.
//
// A change requested by a scene (push/pop/replace/reset) is not applied right away -- the scene
// asking is usually in the middle of its own update(). Instead the screen fades to black over half
// the transition, the change is applied at full black, and the new top fades in. While a change is
// pending, further requests are ignored, so a double key press can't stack two menus.
class SceneManager {
public:
    static constexpr float kDefaultFadeSeconds = 0.3f; // total: half out, half in

    explicit SceneManager(float fadeSeconds = kDefaultFadeSeconds) : m_fadeSeconds(fadeSeconds) {}

    // `instant` skips the fade (used for the very first scene).
    void push(std::unique_ptr<Scene> scene, bool instant = false);
    void pop(bool instant = false);
    void replace(std::unique_ptr<Scene> scene, bool instant = false);
    void reset(std::unique_ptr<Scene> scene, bool instant = false);

    // Advances the fade, applies a pending change at full black, then updates the top scene.
    void update(float frameSeconds);
    void draw() { m_stack.draw(); }

    // Exits and destroys every scene immediately (shutdown).
    void clear();

    // 0 = no fade, 1 = fully black. Drawn by the platform layer over everything.
    float fadeAlpha() const;
    bool transitioning() const { return m_phase != Phase::Idle; }

    const SceneStack& stack() const { return m_stack; }
    bool empty() const { return m_stack.empty(); }

private:
    enum class Op { None, Push, Pop, Replace, Reset };
    enum class Phase { Idle, FadingOut, FadingIn };

    void request(Op op, std::unique_ptr<Scene> scene, bool instant);
    void apply();

    SceneStack m_stack;
    float m_fadeSeconds;
    Phase m_phase = Phase::Idle;
    float m_phaseTime = 0.0f;
    Op m_pendingOp = Op::None;
    std::unique_ptr<Scene> m_pendingScene;
};

} // namespace gridiron
