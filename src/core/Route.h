#pragma once

#include "Vec2.h"

#include <cstddef>
#include <vector>

namespace gridiron {

// Result of moving along a route for one step.
struct RouteStep {
    Vec2 position;
    std::size_t nextWaypoint = 0; // index of the waypoint still being run toward
    bool finished = false;        // true once the last waypoint has been reached
};

// Moves `position` up to `maxDistance` yards along the route, starting toward
// waypoints[nextWaypoint].
//
// Distance left over after reaching a waypoint carries on toward the next one in the same step.
// Without that, a receiver would lose a bit of speed at every cut, and how much would depend on
// the timestep. The function is pure (no side effects), which is what makes it easy to test.
RouteStep followRoute(const std::vector<Vec2>& waypoints, std::size_t nextWaypoint, Vec2 position,
                      float maxDistance);

} // namespace gridiron
