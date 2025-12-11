#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <random>
#include <cmath>
#include "../ecs/components/Mesh.h"

namespace BuildingGenerator {

// Building grid parameters
constexpr int GRID_SIZE = 100;  // 100x100 grid = 10,000 buildings
constexpr float BUILDING_WIDTH = 8.0f;
constexpr float BUILDING_DEPTH = 8.0f;
constexpr float BUILDING_MIN_HEIGHT = 15.0f;
constexpr float BUILDING_MAX_HEIGHT = 40.0f;
constexpr float STREET_WIDTH = 12.0f;
constexpr float BLOCK_SIZE = BUILDING_WIDTH + STREET_WIDTH;  // 20 units per block

// Grid offset so protagonist starts in a "street" intersection (center of grid)
inline float getGridOffsetX() { return -BLOCK_SIZE * GRID_SIZE / 2.0f + STREET_WIDTH / 2.0f; }
inline float getGridOffsetZ() { return -BLOCK_SIZE * GRID_SIZE / 2.0f + STREET_WIDTH / 2.0f; }

struct BuildingData {
    glm::vec3 position;  // Center position at ground level
    float width;
    float depth;
    float height;
    int gridX;  // Grid coordinates for spatial culling
    int gridZ;
};

// Create a unit box mesh (1x1x1) that can be scaled via Transform
// 24 vertices (4 per face) for correct flat-shaded normals
// Returns VAO, VBO, EBO handles and index count
inline Mesh createUnitBoxMesh() {
    // 6 faces, 4 vertices each = 24 vertices
    // Each vertex: position (3) + normal (3) + texcoord (2) = 8 floats
    std::vector<float> vertices = {
        // Front face (Z+)
        -0.5f, 0.0f,  0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 0.0f,
         0.5f, 0.0f,  0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 0.0f,
         0.5f, 1.0f,  0.5f,   0.0f, 0.0f, 1.0f,   1.0f, 1.0f,
        -0.5f, 1.0f,  0.5f,   0.0f, 0.0f, 1.0f,   0.0f, 1.0f,

        // Back face (Z-)
         0.5f, 0.0f, -0.5f,   0.0f, 0.0f, -1.0f,  0.0f, 0.0f,
        -0.5f, 0.0f, -0.5f,   0.0f, 0.0f, -1.0f,  1.0f, 0.0f,
        -0.5f, 1.0f, -0.5f,   0.0f, 0.0f, -1.0f,  1.0f, 1.0f,
         0.5f, 1.0f, -0.5f,   0.0f, 0.0f, -1.0f,  0.0f, 1.0f,

        // Right face (X+)
         0.5f, 0.0f,  0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
         0.5f, 0.0f, -0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
         0.5f, 1.0f, -0.5f,   1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
         0.5f, 1.0f,  0.5f,   1.0f, 0.0f, 0.0f,   0.0f, 1.0f,

        // Left face (X-)
        -0.5f, 0.0f, -0.5f,  -1.0f, 0.0f, 0.0f,   0.0f, 0.0f,
        -0.5f, 0.0f,  0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 0.0f,
        -0.5f, 1.0f,  0.5f,  -1.0f, 0.0f, 0.0f,   1.0f, 1.0f,
        -0.5f, 1.0f, -0.5f,  -1.0f, 0.0f, 0.0f,   0.0f, 1.0f,

        // Top face (Y+)
        -0.5f, 1.0f,  0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 0.0f,
         0.5f, 1.0f,  0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 0.0f,
         0.5f, 1.0f, -0.5f,   0.0f, 1.0f, 0.0f,   1.0f, 1.0f,
        -0.5f, 1.0f, -0.5f,   0.0f, 1.0f, 0.0f,   0.0f, 1.0f,

        // Bottom face (Y-)
        -0.5f, 0.0f, -0.5f,   0.0f, -1.0f, 0.0f,  0.0f, 0.0f,
         0.5f, 0.0f, -0.5f,   0.0f, -1.0f, 0.0f,  1.0f, 0.0f,
         0.5f, 0.0f,  0.5f,   0.0f, -1.0f, 0.0f,  1.0f, 1.0f,
        -0.5f, 0.0f,  0.5f,   0.0f, -1.0f, 0.0f,  0.0f, 1.0f,
    };

    std::vector<unsigned short> indices = {
        // Front
        0, 1, 2,  2, 3, 0,
        // Back
        4, 5, 6,  6, 7, 4,
        // Right
        8, 9, 10,  10, 11, 8,
        // Left
        12, 13, 14,  14, 15, 12,
        // Top
        16, 17, 18,  18, 19, 16,
        // Bottom
        20, 21, 22,  22, 23, 20,
    };

    GLuint vao, vbo, ebo;
    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned short), indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);

    // Normal attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);

    // TexCoord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    Mesh mesh;
    mesh.vao = vao;
    mesh.indexCount = static_cast<int>(indices.size());
    mesh.indexType = GL_UNSIGNED_SHORT;
    mesh.texture = 0;  // No texture - will use solid color

    return mesh;
}

// Generate building data for the grid
// exclusionMin/exclusionMax: AABB bounds (in XZ) to exclude buildings from (with padding)
// Set both to vec2(0) to disable exclusion
inline std::vector<BuildingData> generateBuildingGrid(unsigned int seed,
                                                       glm::vec2 exclusionMin,
                                                       glm::vec2 exclusionMax) {
    std::vector<BuildingData> buildings;
    buildings.reserve(GRID_SIZE * GRID_SIZE);

    std::mt19937 rng(seed);
    std::uniform_real_distribution<float> heightDist(BUILDING_MIN_HEIGHT, BUILDING_MAX_HEIGHT);

    float offsetX = getGridOffsetX();
    float offsetZ = getGridOffsetZ();

    bool hasExclusion = (exclusionMin != exclusionMax);

    for (int z = 0; z < GRID_SIZE; ++z) {
        for (int x = 0; x < GRID_SIZE; ++x) {
            BuildingData building;

            // Position: center of building cell
            building.position.x = offsetX + x * BLOCK_SIZE + BUILDING_WIDTH / 2.0f;
            building.position.y = 0.0f;
            building.position.z = offsetZ + z * BLOCK_SIZE + BUILDING_DEPTH / 2.0f;

            // Skip buildings within exclusion AABB (check if building overlaps exclusion zone)
            if (hasExclusion) {
                // Building bounds in XZ
                float bMinX = building.position.x - BUILDING_WIDTH / 2.0f;
                float bMaxX = building.position.x + BUILDING_WIDTH / 2.0f;
                float bMinZ = building.position.z - BUILDING_DEPTH / 2.0f;
                float bMaxZ = building.position.z + BUILDING_DEPTH / 2.0f;

                // Check AABB overlap in XZ plane
                bool overlapsX = (bMinX <= exclusionMax.x) && (bMaxX >= exclusionMin.x);
                bool overlapsZ = (bMinZ <= exclusionMax.y) && (bMaxZ >= exclusionMin.y);

                if (overlapsX && overlapsZ) {
                    continue;  // Skip this building - it overlaps exclusion zone
                }
            }

            building.width = BUILDING_WIDTH;
            building.depth = BUILDING_DEPTH;
            building.height = heightDist(rng);
            building.gridX = x;
            building.gridZ = z;

            buildings.push_back(building);
        }
    }

    return buildings;
}

// Get player's grid cell from world position
inline std::pair<int, int> getPlayerGridCell(const glm::vec3& playerPos) {
    float offsetX = getGridOffsetX();
    float offsetZ = getGridOffsetZ();

    int gridX = static_cast<int>(std::floor((playerPos.x - offsetX) / BLOCK_SIZE));
    int gridZ = static_cast<int>(std::floor((playerPos.z - offsetZ) / BLOCK_SIZE));

    return {gridX, gridZ};
}

// Check if a building is within render radius of player's grid cell
inline bool isBuildingInRange(const BuildingData& building, int playerGridX, int playerGridZ, int radius) {
    int dx = std::abs(building.gridX - playerGridX);
    int dz = std::abs(building.gridZ - playerGridZ);
    return dx <= radius && dz <= radius;
}

// Get building footprints for minimap rendering
// Returns vector of (center, halfExtents) pairs in world XZ coordinates
inline std::vector<std::pair<glm::vec2, glm::vec2>> getBuildingFootprints(const std::vector<BuildingData>& buildings) {
    std::vector<std::pair<glm::vec2, glm::vec2>> footprints;
    footprints.reserve(buildings.size());

    for (const auto& b : buildings) {
        glm::vec2 center(b.position.x, b.position.z);
        glm::vec2 halfExtents(b.width / 2.0f, b.depth / 2.0f);
        footprints.push_back({center, halfExtents});
    }

    return footprints;
}

}  // namespace BuildingGenerator
