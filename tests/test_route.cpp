#include "core/Route.h"
#include "core/Steering.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace gridiron;
using Catch::Approx;

TEST_CASE("followRoute moves toward the next waypoint by maxDistance", "[route]") {
    const std::vector<Vec2> route = {{10.0f, 0.0f}};
    const RouteStep step = followRoute(route, 0, {0.0f, 0.0f}, 2.5f);

    CHECK(step.position.x == Approx(2.5f));
    CHECK(step.position.y == Approx(0.0f));
    CHECK(step.nextWaypoint == 0);
    CHECK_FALSE(step.finished);
}

TEST_CASE("followRoute never overshoots the final waypoint", "[route]") {
    const std::vector<Vec2> route = {{1.0f, 0.0f}};
    const RouteStep step = followRoute(route, 0, {0.0f, 0.0f}, 5.0f);

    CHECK(step.position == Vec2{1.0f, 0.0f});
    CHECK(step.finished);
}

TEST_CASE("followRoute carries leftover distance around a cut", "[route]") {
    // A 5-yard stem, then a 90-degree cut. 7 yards of travel = 5 up the stem + 2 after the cut.
    const std::vector<Vec2> route = {{5.0f, 0.0f}, {5.0f, 10.0f}};
    const RouteStep step = followRoute(route, 0, {0.0f, 0.0f}, 7.0f);

    CHECK(step.position.x == Approx(5.0f));
    CHECK(step.position.y == Approx(2.0f));
    CHECK(step.nextWaypoint == 1);
    CHECK_FALSE(step.finished);
}

TEST_CASE("followRoute total distance is independent of step size", "[route]") {
    const std::vector<Vec2> route = {{5.0f, 0.0f}, {5.0f, 5.0f}, {12.0f, 5.0f}};

    RouteStep oneBigStep = followRoute(route, 0, {}, 9.0f);

    RouteStep manySmallSteps{{}, 0, false};
    for (int i = 0; i < 90; ++i) {
        manySmallSteps =
            followRoute(route, manySmallSteps.nextWaypoint, manySmallSteps.position, 0.1f);
    }
    CHECK(manySmallSteps.position.x == Approx(oneBigStep.position.x).margin(1e-4));
    CHECK(manySmallSteps.position.y == Approx(oneBigStep.position.y).margin(1e-4));
}

TEST_CASE("followRoute on an empty or finished route stays put", "[route]") {
    const std::vector<Vec2> empty;
    const RouteStep step = followRoute(empty, 0, {3.0f, 4.0f}, 1.0f);
    CHECK(step.position == Vec2{3.0f, 4.0f});
    CHECK(step.finished);
}

TEST_CASE("Steering: arrive slows down inside the slow radius", "[steering]") {
    const Vec2 far = steering::arrive({0.0f, 0.0f}, {10.0f, 0.0f}, 8.0f, 2.0f);
    const Vec2 near = steering::arrive({9.0f, 0.0f}, {10.0f, 0.0f}, 8.0f, 2.0f);
    CHECK(length(far) == Approx(8.0f));
    CHECK(length(near) == Approx(4.0f));
}

TEST_CASE("Steering: pursuit leads a moving target", "[steering]") {
    // Target 10 yards ahead moving up at 5 yd/s; pursuer at 10 yd/s needs ~1 s to get there.
    const Vec2 aim =
        steering::predictInterceptPoint({0.0f, 0.0f}, 10.0f, {10.0f, 0.0f}, {0.0f, 5.0f}, 2.0f);
    CHECK(aim.x == Approx(10.0f));
    CHECK(aim.y == Approx(5.0f));
}

TEST_CASE("Steering: acceleration is limited per step", "[steering]") {
    const Vec2 v = steering::accelerateToward({0.0f, 0.0f}, {10.0f, 0.0f}, 12.0f, 0.5f);
    CHECK(v.x == Approx(6.0f)); // 12 yd/s^2 * 0.5 s
}
