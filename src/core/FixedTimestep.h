#pragma once

namespace gridiron {

// Converts variable real-time frame durations into a whole number of fixed simulation steps.
//
// Why: the simulation always advances in identical 1/60 s steps, so its behavior does not
// depend on the monitor's refresh rate or on frame hitches, and tests can reproduce it exactly.
// Rendering then blends the last two simulation states using alpha() (see App::render).
//
// Usage per rendered frame:
//   const int steps = timestep.advance(frameSeconds);
//   for (int i = 0; i < steps; ++i) simulate(timestep.stepSeconds());
//   render(timestep.alpha());
class FixedTimestep {
public:
    // maxStepsPerFrame guards against the "spiral of death": if a frame takes very long (a
    // breakpoint, a window drag), we drop the backlog instead of trying to catch up forever.
    explicit FixedTimestep(double stepSeconds, int maxStepsPerFrame = 5);

    // Adds real elapsed time and returns how many fixed steps the caller should run now.
    int advance(double frameSeconds);

    // How far we are between the previous and the current simulation state, in [0, 1).
    double alpha() const { return m_accumulator / m_stepSeconds; }

    double stepSeconds() const { return m_stepSeconds; }

private:
    double m_stepSeconds;
    int m_maxStepsPerFrame;
    double m_accumulator = 0.0; // real time not yet consumed by a simulation step
};

} // namespace gridiron
