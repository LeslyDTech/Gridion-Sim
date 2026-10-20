#include "FixedTimestep.h"

#include <algorithm>

namespace gridiron {

FixedTimestep::FixedTimestep(double stepSeconds, int maxStepsPerFrame)
    : m_stepSeconds(stepSeconds), m_maxStepsPerFrame(std::max(1, maxStepsPerFrame)) {}

int FixedTimestep::advance(double frameSeconds) {
    m_accumulator += std::max(0.0, frameSeconds);

    int steps = 0;
    while (m_accumulator >= m_stepSeconds && steps < m_maxStepsPerFrame) {
        m_accumulator -= m_stepSeconds;
        ++steps;
    }

    // Still a full step behind after the maximum? Drop the backlog so we don't fall further
    // behind every frame. The game briefly runs in slow motion instead of freezing.
    if (m_accumulator >= m_stepSeconds) {
        m_accumulator = 0.0;
    }
    return steps;
}

} // namespace gridiron
