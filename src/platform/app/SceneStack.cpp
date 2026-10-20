#include "SceneStack.h"

#include <utility>

namespace gridiron {

SceneStack::~SceneStack() {
    while (!m_scenes.empty()) {
        pop();
    }
}

void SceneStack::push(std::unique_ptr<Scene> scene) {
    if (!scene) {
        return;
    }
    m_scenes.push_back(std::move(scene));
    m_scenes.back()->enter();
}

void SceneStack::pop() {
    if (m_scenes.empty()) {
        return;
    }
    m_scenes.back()->exit();
    m_scenes.pop_back();
}

void SceneStack::replace(std::unique_ptr<Scene> scene) {
    pop();
    push(std::move(scene));
}

void SceneStack::reset(std::unique_ptr<Scene> scene) {
    while (!m_scenes.empty()) {
        pop();
    }
    push(std::move(scene));
}

void SceneStack::updateTop(float frameSeconds) {
    if (Scene* scene = top()) {
        scene->update(frameSeconds);
    }
}

void SceneStack::draw() {
    if (m_scenes.empty()) {
        return;
    }
    // Walk down past overlays to the first opaque scene, then draw back up.
    std::size_t first = m_scenes.size() - 1;
    while (first > 0 && m_scenes[first]->isOverlay()) {
        --first;
    }
    for (std::size_t i = first; i < m_scenes.size(); ++i) {
        m_scenes[i]->draw();
    }
}

} // namespace gridiron
