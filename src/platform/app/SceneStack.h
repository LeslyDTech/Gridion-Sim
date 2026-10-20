#pragma once

#include "Scene.h"

#include <cstddef>
#include <memory>
#include <vector>

namespace gridiron {

// Owns the scenes, top of the stack last. Operations take effect immediately and call
// enter()/exit() in a well-defined order; SceneManager decides *when* they run (between frames,
// halfway through a fade).
class SceneStack {
public:
    SceneStack() = default;
    SceneStack(const SceneStack&) = delete;
    SceneStack& operator=(const SceneStack&) = delete;
    ~SceneStack(); // exits remaining scenes, top first

    void push(std::unique_ptr<Scene> scene);    // new scene enters on top
    void pop();                                 // top exits and is destroyed
    void replace(std::unique_ptr<Scene> scene); // top exits, new scene enters in its place
    void reset(std::unique_ptr<Scene> scene);   // every scene exits (top first), new one enters

    Scene* top() const { return m_scenes.empty() ? nullptr : m_scenes.back().get(); }
    std::size_t size() const { return m_scenes.size(); }
    bool empty() const { return m_scenes.empty(); }

    void updateTop(float frameSeconds);

    // Draws from the highest non-overlay scene up to the top.
    void draw();

private:
    std::vector<std::unique_ptr<Scene>> m_scenes;
};

} // namespace gridiron
