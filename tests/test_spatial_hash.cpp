#include "core/Collision.h"
#include "core/SpatialHash.h"

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <random>
#include <set>
#include <vector>

using namespace gridiron;
using Pair = SpatialHash::Pair;

namespace {

SpatialHash makeGrid() { return SpatialHash({0.0f, 0.0f}, {20.0f, 10.0f}, 2.0f); }

// Pairs normalized to (min, max) so comparisons ignore order.
std::set<Pair> asSet(const std::vector<Pair>& pairs) {
    std::set<Pair> set;
    for (const auto& [a, b] : pairs) {
        set.emplace(std::min(a, b), std::max(a, b));
    }
    return set;
}

} // namespace

TEST_CASE("SpatialHash maps positions to cells and clamps outside points", "[spatial]") {
    const SpatialHash grid = makeGrid();
    CHECK(grid.columns() == 10);
    CHECK(grid.rows() == 5);
    CHECK(grid.column(0.0f) == 0);
    CHECK(grid.column(3.9f) == 1);
    CHECK(grid.row(9.99f) == 4);
    CHECK(grid.column(-50.0f) == 0); // clamped into the border
    CHECK(grid.row(500.0f) == 4);
}

TEST_CASE("SpatialHash pairs entities in the same cell", "[spatial]") {
    SpatialHash grid = makeGrid();
    grid.insert(0, {1.0f, 1.0f});
    grid.insert(1, {1.5f, 1.5f});

    std::vector<Pair> pairs;
    grid.findCandidatePairs(pairs);
    CHECK(asSet(pairs) == std::set<Pair>{{0, 1}});
}

TEST_CASE("SpatialHash pairs entities across a cell border, including diagonals", "[spatial]") {
    SpatialHash grid = makeGrid();
    grid.insert(0, {1.9f, 1.9f}); // cell (0, 0)
    grid.insert(1, {2.1f, 1.9f}); // cell (1, 0), right
    grid.insert(2, {2.1f, 2.1f}); // cell (1, 1), diagonal down-right
    grid.insert(3, {1.9f, 2.1f}); // cell (0, 1), below

    std::vector<Pair> pairs;
    grid.findCandidatePairs(pairs);
    CHECK(asSet(pairs) ==
          std::set<Pair>{{0, 1}, {0, 2}, {0, 3}, {1, 2}, {1, 3}, {2, 3}}); // all 6 pairs
}

TEST_CASE("SpatialHash ignores entities two or more cells apart", "[spatial]") {
    SpatialHash grid = makeGrid();
    grid.insert(0, {1.0f, 1.0f});
    grid.insert(1, {9.0f, 1.0f});
    grid.insert(2, {1.0f, 9.0f});

    std::vector<Pair> pairs;
    grid.findCandidatePairs(pairs);
    CHECK(pairs.empty());
}

TEST_CASE("SpatialHash never reports a pair twice", "[spatial]") {
    SpatialHash grid = makeGrid();
    // A dense cluster spanning four cells.
    int id = 0;
    for (float x = 1.0f; x < 3.0f; x += 0.5f) {
        for (float y = 1.0f; y < 3.0f; y += 0.5f) {
            grid.insert(id++, {x, y});
        }
    }
    std::vector<Pair> pairs;
    grid.findCandidatePairs(pairs);
    CHECK(asSet(pairs).size() == pairs.size());
    CHECK(pairs.size() == static_cast<std::size_t>(id * (id - 1) / 2)); // all are neighbors
}

TEST_CASE("SpatialHash finds every overlapping pair brute force finds", "[spatial]") {
    // Randomized cross-check against the O(n^2) answer: the broad phase may report extra
    // candidates, but must never miss a real overlap.
    std::mt19937 rng(1234); // fixed seed, so the test is reproducible
    std::uniform_real_distribution<float> x(0.0f, 20.0f);
    std::uniform_real_distribution<float> y(0.0f, 10.0f);
    constexpr float kRadius = 0.65f; // cell size 2.0 >= diameter 1.3

    std::vector<Vec2> points(40);
    for (Vec2& p : points) {
        p = {x(rng), y(rng)};
    }

    SpatialHash grid = makeGrid();
    for (int i = 0; i < static_cast<int>(points.size()); ++i) {
        grid.insert(i, points[static_cast<std::size_t>(i)]);
    }
    std::vector<Pair> candidates;
    grid.findCandidatePairs(candidates);
    const std::set<Pair> candidateSet = asSet(candidates);

    for (int i = 0; i < static_cast<int>(points.size()); ++i) {
        for (int j = i + 1; j < static_cast<int>(points.size()); ++j) {
            const bool overlap = findContact({points[static_cast<std::size_t>(i)], kRadius},
                                             {points[static_cast<std::size_t>(j)], kRadius})
                                     .has_value();
            if (overlap) {
                CHECK(candidateSet.count({i, j}) == 1);
            }
        }
    }
    CHECK(candidates.size() < points.size() * (points.size() - 1) / 2); // and it prunes
}

TEST_CASE("SpatialHash clear empties every cell", "[spatial]") {
    SpatialHash grid = makeGrid();
    grid.insert(0, {1.0f, 1.0f});
    grid.insert(1, {1.2f, 1.0f});
    grid.clear();
    CHECK(grid.countInCell(0, 0) == 0);

    std::vector<Pair> pairs{{7, 8}}; // stale content must be replaced
    grid.findCandidatePairs(pairs);
    CHECK(pairs.empty());
}

TEST_CASE("Heavier players take less of the push-apart", "[collision]") {
    const SeparationShares lineman = separationShares(1.5f, 0.85f); // lineman vs receiver
    CHECK(lineman.a + lineman.b == Catch::Approx(1.0f));
    CHECK(lineman.a < lineman.b); // the lineman moves less
    const SeparationShares equal = separationShares(1.0f, 1.0f);
    CHECK(equal.a == Catch::Approx(0.5f));
}

TEST_CASE("findContact reports overlap depth and direction", "[collision]") {
    const auto contact = findContact({{0.0f, 0.0f}, 1.0f}, {{1.5f, 0.0f}, 1.0f});
    REQUIRE(contact.has_value());
    CHECK(contact->penetration == Catch::Approx(0.5f));
    CHECK(contact->normal == Vec2{1.0f, 0.0f});

    CHECK_FALSE(findContact({{0.0f, 0.0f}, 1.0f}, {{2.0f, 0.0f}, 1.0f}).has_value()); // touching
}
