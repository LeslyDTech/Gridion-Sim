#pragma once

#include "Vec2.h"

#include <utility>
#include <vector>

namespace gridiron {

// Uniform-grid broad phase for collision detection.
//
// Checking every pair of 22 players is 231 circle tests per step. The grid stores each player in
// the cell containing their center; only players in the same or adjacent cells can touch, so far
// fewer pairs reach the exact circle test. A grid suits this game better than a quadtree or BVH:
// players are similar in size, the field is a fixed rectangle, and rebuilding is trivially cheap.
//
// Requirement: cellSize >= the largest player diameter. Then two touching circles always have
// their centers in the same or neighboring cells.
//
// No allocations after warm-up: clear() keeps every vector's capacity, so after the first few
// frames inserting and querying only reuse memory.
class SpatialHash {
public:
    using Pair = std::pair<int, int>;

    SpatialHash(Vec2 worldMin, Vec2 worldMax, float cellSize);

    void clear();
    void insert(int id, Vec2 position);

    // Replaces `out` with every candidate pair (a, b), a != b, whose cells are the same or
    // adjacent. Each unordered pair appears exactly once.
    void findCandidatePairs(std::vector<Pair>& out) const;

    // --- For tests and the debug view ---
    int columns() const { return m_columns; }
    int rows() const { return m_rows; }
    float cellSize() const { return m_cellSize; }
    Vec2 origin() const { return m_origin; }
    int column(float x) const;
    int row(float y) const;
    int countInCell(int column, int row) const;

private:
    int cellIndex(int column, int row) const { return row * m_columns + column; }
    void addPairsBetween(int cellA, int cellB, std::vector<Pair>& out) const;

    Vec2 m_origin;
    float m_cellSize;
    int m_columns;
    int m_rows;
    std::vector<std::vector<int>> m_cells; // player ids per cell
    std::vector<int> m_occupiedCells; // indices of non-empty cells, so clear/query skip empties
};

} // namespace gridiron
