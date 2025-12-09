#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <cmath>

// Hash function for grid coordinates
struct GridPosHash {
    size_t operator()(const std::pair<int, int>& p) const {
        return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 16);
    }
};

struct DynamicTerrain {
    // Grid dimensions
    int gridSize = 32;           // Number of vertices per side (32x32 grid)
    float cellSize = 0.25f;      // Size of each cell in world units
    float totalSize() const { return (gridSize - 1) * cellSize; }

    // Follow target
    Entity followTarget = NULL_ENTITY;

    // OpenGL buffers (will be created dynamically)
    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    // Vertex data: position (3), normal (3), color (3) = 9 floats per vertex
    std::vector<float> vertices;
    std::vector<unsigned int> indices;

    // Height map for current grid (recomputed from world deformations)
    std::vector<float> heights;

    // PERSISTENT world-space deformations: key = (worldGridX, worldGridZ), value = height offset
    std::unordered_map<std::pair<int, int>, float, GridPosHash> worldDeformations;

    // Colors
    glm::vec3 baseColor = glm::vec3(1.0f, 0.0f, 0.0f);  // Red for debug
    glm::vec3 holeColor = glm::vec3(0.5f, 0.0f, 0.5f);  // Purple for holes

    bool needsRebuild = true;
    bool initialized = false;

    void init() {
        if (initialized) return;

        int vertexCount = gridSize * gridSize;
        heights.resize(vertexCount, 0.0f);

        // 9 floats per vertex: pos(3) + normal(3) + color(3)
        vertices.resize(vertexCount * 9);

        // Calculate index count: (gridSize-1)^2 quads * 2 triangles * 3 indices
        int quadCount = (gridSize - 1) * (gridSize - 1);
        indices.resize(quadCount * 6);

        // Generate indices (these don't change)
        int idx = 0;
        for (int z = 0; z < gridSize - 1; z++) {
            for (int x = 0; x < gridSize - 1; x++) {
                int topLeft = z * gridSize + x;
                int topRight = topLeft + 1;
                int bottomLeft = (z + 1) * gridSize + x;
                int bottomRight = bottomLeft + 1;

                // First triangle
                indices[idx++] = topLeft;
                indices[idx++] = bottomLeft;
                indices[idx++] = topRight;

                // Second triangle
                indices[idx++] = topRight;
                indices[idx++] = bottomLeft;
                indices[idx++] = bottomRight;
            }
        }

        // Create OpenGL buffers
        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), nullptr, GL_DYNAMIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        // Position (location 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);

        // Normal (location 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);

        // Color (location 2) - we'll use this instead of UV for terrain
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);

        initialized = true;
        needsRebuild = true;
    }

    // Convert world position to grid cell index
    int worldToGridIndex(float worldCoord) const {
        return static_cast<int>(std::floor(worldCoord / cellSize));
    }

    void rebuildMesh(const glm::vec3& centerPos) {
        if (!initialized) return;

        float halfSize = totalSize() * 0.5f;

        // First pass: load heights from world deformations and set positions
        int vertIdx = 0;
        for (int z = 0; z < gridSize; z++) {
            for (int x = 0; x < gridSize; x++) {
                // Calculate world position of this vertex
                float worldX = centerPos.x - halfSize + x * cellSize;
                float worldZ = centerPos.z - halfSize + z * cellSize;

                // Convert to world grid index
                int worldGridX = worldToGridIndex(worldX);
                int worldGridZ = worldToGridIndex(worldZ);

                // Look up persistent deformation at this world position
                auto key = std::make_pair(worldGridX, worldGridZ);
                auto it = worldDeformations.find(key);
                float height = (it != worldDeformations.end()) ? it->second : 0.0f;

                // Store in local heights array for normal calculation
                int heightIdx = z * gridSize + x;
                heights[heightIdx] = height;

                // Position - raise terrain slightly above white plane to prevent z-fighting
                float worldY = height + 0.005f;

                vertices[vertIdx] = worldX;
                vertices[vertIdx + 1] = worldY;
                vertices[vertIdx + 2] = worldZ;

                vertIdx += 9;  // Skip to next vertex (pos + normal + color = 9 floats)
            }
        }

        // Second pass: calculate normals from height differences
        vertIdx = 0;
        for (int z = 0; z < gridSize; z++) {
            for (int x = 0; x < gridSize; x++) {
                // Get heights of neighboring vertices for normal calculation
                float hL = getHeight(x - 1, z);
                float hR = getHeight(x + 1, z);
                float hD = getHeight(x, z - 1);
                float hU = getHeight(x, z + 1);

                // Calculate normal from height gradient
                glm::vec3 normal;
                normal.x = (hL - hR) / (2.0f * cellSize);
                normal.y = 1.0f;
                normal.z = (hD - hU) / (2.0f * cellSize);
                normal = glm::normalize(normal);

                vertices[vertIdx + 3] = normal.x;
                vertices[vertIdx + 4] = normal.y;
                vertices[vertIdx + 5] = normal.z;

                // Color based on height (PURE RED in holes for debugging)
                float height = heights[z * gridSize + x];
                glm::vec3 color;
                if (height < -0.01f) {
                    color = glm::vec3(1.0f, 0.0f, 0.0f);  // Pure red for ANY deformation
                } else {
                    color = glm::vec3(1.0f, 1.0f, 1.0f);  // White otherwise
                }
                vertices[vertIdx + 6] = color.r;
                vertices[vertIdx + 7] = color.g;
                vertices[vertIdx + 8] = color.b;

                vertIdx += 9;
            }
        }

        // Upload to GPU
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());

        needsRebuild = false;
    }

    // Helper to get height with boundary clamping
    float getHeight(int x, int z) const {
        x = glm::clamp(x, 0, gridSize - 1);
        z = glm::clamp(z, 0, gridSize - 1);
        return heights[z * gridSize + x];
    }

    void cleanup() {
        if (vao) {
            glDeleteVertexArrays(1, &vao);
            vao = 0;
        }
        if (vbo) {
            glDeleteBuffers(1, &vbo);
            vbo = 0;
        }
        if (ebo) {
            glDeleteBuffers(1, &ebo);
            ebo = 0;
        }
        initialized = false;
    }

    // Apply a deformation at world position - stores in persistent world grid
    void deformAt(const glm::vec3& worldPos, float radius, float depth) {
        // Calculate the grid cell range affected by this deformation
        int radiusCells = static_cast<int>(std::ceil(radius / cellSize)) + 1;
        int centerGridX = worldToGridIndex(worldPos.x);
        int centerGridZ = worldToGridIndex(worldPos.z);

        for (int dz = -radiusCells; dz <= radiusCells; dz++) {
            for (int dx = -radiusCells; dx <= radiusCells; dx++) {
                int worldGridX = centerGridX + dx;
                int worldGridZ = centerGridZ + dz;

                // Calculate world position of this grid cell center
                float cellWorldX = worldGridX * cellSize + cellSize * 0.5f;
                float cellWorldZ = worldGridZ * cellSize + cellSize * 0.5f;

                float distX = cellWorldX - worldPos.x;
                float distZ = cellWorldZ - worldPos.z;
                float dist = std::sqrt(distX * distX + distZ * distZ);

                if (dist < radius) {
                    float factor = 1.0f - (dist / radius);
                    factor = factor * factor;  // Smooth falloff
                    float deformHeight = -depth * factor;

                    // Store in persistent world map (keep minimum/deepest value)
                    auto key = std::make_pair(worldGridX, worldGridZ);
                    auto it = worldDeformations.find(key);
                    if (it != worldDeformations.end()) {
                        it->second = std::min(it->second, deformHeight);
                    } else {
                        worldDeformations[key] = deformHeight;
                    }
                }
            }
        }
        needsRebuild = true;
    }

    // Reset heights when moving to new area
    void resetHeights() {
        std::fill(heights.begin(), heights.end(), 0.0f);
        needsRebuild = true;
    }
};
