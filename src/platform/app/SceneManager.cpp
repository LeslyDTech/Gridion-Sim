#include "SceneManager.h"

#include <algorithm>
#include <utility>

namespace gridiron {

void SceneManager::push(std::unique_ptr<Scene> scene, bool instant) {
    request(Op::Push, std::move(scene), instant);
}
void SceneManager::pop(bool instant) { request(Op::Pop, nullptr, instant); }
void SceneManager::replace(std::unique_ptr<Scene> scene, bool instant) {
    request(Op::Replace, std::move(scene), instant);
}
void SceneManager::reset(std::unique_ptr<Scene> scene, bool instant) {
    request(Op::Reset, std::move(scene), instant);
}

void SceneManager::request(Op op, std::unique_ptr<Scene> scene, bool instant) {
    if (m_pendingOp != Op::None || m_phase != Phase::Idle) {
        return; // one change at a time
    }
    m_pendingOp = op;
    m_pendingScene = std::move(scene);
    if (instant || m_fadeSeconds <= 0.0f) {
        apply();
        return;
    }
    m_phase = Phase::FadingOut;
    m_phaseTime = 0.0f;
}

void SceneManager::apply() {
    switch (m_pendingOp) {
    case Op::Push:
        m_stack.push(std::move(m_pendingScene));
        break;
    case Op::Pop:
        m_stack.pop();
        break;
    case Op::Replace:
        m_stack.replace(std::move(m_pendingScene));
        break;
    case Op::Reset:
        m_stack.reset(std::move(m_pendingScene));
        break;
    case Op::None:
        break;
    }
    m_pendingOp = Op::None;
    m_pendingScene.reset();
}

void SceneManager::update(float frameSeconds) {
    const float half = m_fadeSeconds / 2.0f;
    if (m_phase == Phase::FadingOut) {
        m_phaseTime += frameSeconds;
        if (m_phaseTime >= half) {
            apply(); // at full black, so the swap is never visible
            m_phase = Phase::FadingIn;
            m_phaseTime = 0.0f;
        }
    } else if (m_phase == Phase::FadingIn) {
        m_phaseTime += frameSeconds;
        if (m_phaseTime >= half) {
            m_phase = Phase::Idle;
            m_phaseTime = 0.0f;
        }
    }
    m_stack.updateTop(frameSeconds);
}

void SceneManager::clear() {
    m_pendingOp = Op::None;
    m_pendingScene.reset();
    m_phase = Phase::Idle;
    m_stack.reset(nullptr);
}

float SceneManager::fadeAlpha() const {
    const float half = m_fadeSeconds / 2.0f;
    switch (m_phase) {
    case Phase::FadingOut:
        return std::clamp(m_phaseTime / half, 0.0f, 1.0f);
    case Phase::FadingIn:
        return std::clamp(1.0f - m_phaseTime / half, 0.0f, 1.0f);
    case Phase::Idle:
        break;
    }
    return 0.0f;
}

} // namespace gridiron
