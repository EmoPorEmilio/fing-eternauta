#pragma once
#include "../ecs/components/AABB.h"
#include "../ecs/Registry.h"
#include <vector>
#include <cmath>
#include <algorithm>

// Simple spatial grid for efficient distance-based culling
// Optimized for large grids where we only render nearby entities
class SpatialGrid {
public:
    struct GridCell {
        int gridX;
        int gridZ;
        Entity entity;
        glm::vec3 position;
    };

    void clear() {
        m_cells.clear();
        m_gridWidth = 0;
        m_gridHeight = 0;
    }

    void addEntity(Entity entity, const glm::vec3& position, int gridX, int gridZ) {
        m_cells.push_back({gridX, gridZ, entity, position});
        m_gridWidth = std::max(m_gridWidth, gridX + 1);
        m_gridHeight = std::max(m_gridHeight, gridZ + 1);
    }

    // Get entities within a certain grid radius from the player's grid cell
    std::vector<Entity> getEntitiesInRadius(const glm::vec3& playerPos, float blockSize, int cellRadius) const {
        std::vector<Entity> result;
        result.reserve((2 * cellRadius + 1) * (2 * cellRadius + 1));

        // Calculate player's grid cell
        int playerGridX = static_cast<int>(std::floor((playerPos.x + m_gridWidth * blockSize / 2.0f) / blockSize));
        int playerGridZ = static_cast<int>(std::floor((playerPos.z + m_gridHeight * blockSize / 2.0f) / blockSize));

        // Collect entities within the radius
        for (const auto& cell : m_cells) {
            int dx = std::abs(cell.gridX - playerGridX);
            int dz = std::abs(cell.gridZ - playerGridZ);

            // Check if within the grid cell radius (Chebyshev distance)
            if (dx <= cellRadius && dz <= cellRadius) {
                result.push_back(cell.entity);
            }
        }

        return result;
    }

    // Get entities sorted by distance from player (for potential LOD)
    std::vector<std::pair<Entity, float>> getEntitiesSortedByDistance(const glm::vec3& playerPos, int maxCount = -1) const {
        std::vector<std::pair<Entity, float>> result;
        result.reserve(m_cells.size());

        for (const auto& cell : m_cells) {
            float dist = glm::length(cell.position - playerPos);
            result.push_back({cell.entity, dist});
        }

        std::sort(result.begin(), result.end(),
            [](const auto& a, const auto& b) { return a.second < b.second; });

        if (maxCount > 0 && result.size() > static_cast<size_t>(maxCount)) {
            result.resize(maxCount);
        }

        return result;
    }

    size_t totalEntities() const { return m_cells.size(); }

private:
    std::vector<GridCell> m_cells;
    int m_gridWidth = 0;
    int m_gridHeight = 0;
};
