#include "SpatialHash.h"

#include <algorithm>
#include <cmath>

namespace gridiron {
namespace {

// Room for a typical crowd (a pile of linemen) per cell before a vector ever has to grow.
constexpr std::size_t kInitialCellCapacity = 8;

int cellCount(float extent, float cellSize) {
    return std::max(1, static_cast<int>(std::ceil(extent / cellSize)));
}

} // namespace

SpatialHash::SpatialHash(Vec2 worldMin, Vec2 worldMax, float cellSize)
    : m_origin(worldMin), m_cellSize(cellSize),
      m_columns(cellCount(worldMax.x - worldMin.x, cellSize)),
      m_rows(cellCount(worldMax.y - worldMin.y, cellSize)),
      m_cells(static_cast<std::size_t>(m_columns * m_rows)) {
    for (std::vector<int>& cell : m_cells) {
        cell.reserve(kInitialCellCapacity);
    }
    m_occupiedCells.reserve(m_cells.size());
}

int SpatialHash::column(float x) const {
    // Anything outside the grid is clamped into the border cells, so it is still checked.
    return std::clamp(static_cast<int>(std::floor((x - m_origin.x) / m_cellSize)), 0,
                      m_columns - 1);
}

int SpatialHash::row(float y) const {
    return std::clamp(static_cast<int>(std::floor((y - m_origin.y) / m_cellSize)), 0, m_rows - 1);
}

int SpatialHash::countInCell(int col, int r) const {
    return static_cast<int>(m_cells[static_cast<std::size_t>(cellIndex(col, r))].size());
}

void SpatialHash::clear() {
    for (const int index : m_occupiedCells) {
        m_cells[static_cast<std::size_t>(index)].clear(); // keeps capacity
    }
    m_occupiedCells.clear();
}

void SpatialHash::insert(int id, Vec2 position) {
    const int index = cellIndex(column(position.x), row(position.y));
    std::vector<int>& cell = m_cells[static_cast<std::size_t>(index)];
    if (cell.empty()) {
        m_occupiedCells.push_back(index);
    }
    cell.push_back(id);
}

void SpatialHash::addPairsBetween(int cellA, int cellB, std::vector<Pair>& out) const {
    for (const int a : m_cells[static_cast<std::size_t>(cellA)]) {
        for (const int b : m_cells[static_cast<std::size_t>(cellB)]) {
            out.emplace_back(a, b);
        }
    }
}

void SpatialHash::findCandidatePairs(std::vector<Pair>& out) const {
    out.clear();

    // Visit each occupied cell, pairing it with itself and with only 4 of its 8 neighbors: right,
    // and the three below. The other 4 neighbors pair with this cell when *they* are visited, so
    // every pair of adjacent cells is handled once and no pair is ever reported twice.
    constexpr int kForwardNeighbors[4][2] = {{1, 0}, {-1, 1}, {0, 1}, {1, 1}};

    for (const int index : m_occupiedCells) {
        const int col = index % m_columns;
        const int r = index / m_columns;

        const std::vector<int>& cell = m_cells[static_cast<std::size_t>(index)];
        for (std::size_t i = 0; i < cell.size(); ++i) {
            for (std::size_t j = i + 1; j < cell.size(); ++j) {
                out.emplace_back(cell[i], cell[j]);
            }
        }

        for (const auto& offset : kForwardNeighbors) {
            const int neighborCol = col + offset[0];
            const int neighborRow = r + offset[1];
            if (neighborCol < 0 || neighborCol >= m_columns || neighborRow >= m_rows) {
                continue;
            }
            addPairsBetween(index, cellIndex(neighborCol, neighborRow), out);
        }
    }
}

} // namespace gridiron
