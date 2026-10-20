#include "Route.h"

namespace gridiron {

RouteStep followRoute(const std::vector<Vec2>& waypoints, std::size_t nextWaypoint, Vec2 position,
                      float maxDistance) {
    float remaining = maxDistance;
    while (remaining > 0.0f && nextWaypoint < waypoints.size()) {
        const Vec2 toWaypoint = waypoints[nextWaypoint] - position;
        const float distanceToWaypoint = length(toWaypoint);

        if (distanceToWaypoint <= remaining) {
            // Reach this waypoint and keep the leftover distance for the next leg.
            position = waypoints[nextWaypoint];
            remaining -= distanceToWaypoint;
            ++nextWaypoint;
        } else {
            position += toWaypoint * (remaining / distanceToWaypoint);
            remaining = 0.0f;
        }
    }
    return {position, nextWaypoint, nextWaypoint >= waypoints.size()};
}

} // namespace gridiron
