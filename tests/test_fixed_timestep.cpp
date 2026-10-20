#include "core/FixedTimestep.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using gridiron::FixedTimestep;

namespace {
constexpr double kStep = 1.0 / 60.0;
}

TEST_CASE("FixedTimestep runs one step per step-length of real time", "[timestep]") {
    FixedTimestep timestep(kStep);
    CHECK(timestep.advance(kStep) == 1);
    CHECK(timestep.advance(kStep * 3.0) == 3);
}

TEST_CASE("FixedTimestep carries leftover time into alpha", "[timestep]") {
    FixedTimestep timestep(kStep);

    CHECK(timestep.advance(kStep * 0.5) == 0);
    CHECK(timestep.alpha() == Catch::Approx(0.5));

    // The leftover half step plus another full step is 1.5 steps: run one, keep half.
    CHECK(timestep.advance(kStep) == 1);
    CHECK(timestep.alpha() == Catch::Approx(0.5));
}

TEST_CASE("FixedTimestep caps steps per frame and drops the backlog", "[timestep]") {
    FixedTimestep timestep(kStep, 5);

    CHECK(timestep.advance(1.0) == 5); // a one-second hitch would be 60 steps
    CHECK(timestep.alpha() == Catch::Approx(0.0));
    CHECK(timestep.advance(kStep) == 1); // back to normal on the next frame
}

TEST_CASE("FixedTimestep ignores negative frame times", "[timestep]") {
    FixedTimestep timestep(kStep);
    CHECK(timestep.advance(-1.0) == 0);
    CHECK(timestep.alpha() == Catch::Approx(0.0));
}
