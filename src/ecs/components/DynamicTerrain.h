#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>
#include <cmath>

struct GridPosHash {
    size_t operator()(const std::pair<int, int>& p) const {
        return std::hash<int>()(p.first) ^ (std::hash<int>()(p.second) << 16);
    }
};

struct DynamicTerrain {
    int gridSize = 32;
    float cellSize = 0.25f;
    float totalSize() const { return (gridSize - 1) * cellSize; }

    Entity followTarget = NULL_ENTITY;

    GLuint vao = 0;
    GLuint vbo = 0;
    GLuint ebo = 0;

    std::vector<float> vertices;   // pos(3) + normal(3) + color(3) per vertex
    std::vector<unsigned int> indices;
    std::vector<float> heights;

    // Persistent world-space deformations: (worldGridX, worldGridZ) -> height offset
    std::unordered_map<std::pair<int, int>, float, GridPosHash> worldDeformations;

    glm::vec3 baseColor = glm::vec3(1.0f, 0.0f, 0.0f);
    glm::vec3 holeColor = glm::vec3(0.5f, 0.0f, 0.5f);

    bool needsRebuild = true;
    bool initialized = false;

    void init() {
        if (initialized) return;

        int vertexCount = gridSize * gridSize;
        heights.resize(vertexCount, 0.0f);
        vertices.resize(vertexCount * 9);

        int quadCount = (gridSize - 1) * (gridSize - 1);
        indices.resize(quadCount * 6);

        int idx = 0;
        for (int z = 0; z < gridSize - 1; z++) {
            for (int x = 0; x < gridSize - 1; x++) {
                int topLeft = z * gridSize + x;
                int topRight = topLeft + 1;
                int bottomLeft = (z + 1) * gridSize + x;
                int bottomRight = bottomLeft + 1;

                indices[idx++] = topLeft;
                indices[idx++] = bottomLeft;
                indices[idx++] = topRight;
                indices[idx++] = topRight;
                indices[idx++] = bottomLeft;
                indices[idx++] = bottomRight;
            }
        }

        glGenVertexArrays(1, &vao);
        glGenBuffers(1, &vbo);
        glGenBuffers(1, &ebo);

        glBindVertexArray(vao);
        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(float), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(unsigned int), indices.data(), GL_STATIC_DRAW);

        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, 9 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
        initialized = true;
        needsRebuild = true;
    }

    int worldToGridIndex(float worldCoord) const {
        return static_cast<int>(std::floor(worldCoord / cellSize));
    }

    void rebuildMesh(const glm::vec3& centerPos) {
        if (!initialized) return;

        float halfSize = totalSize() * 0.5f;

        // First pass: load heights and set positions
        int vertIdx = 0;
        for (int z = 0; z < gridSize; z++) {
            for (int x = 0; x < gridSize; x++) {
                float worldX = centerPos.x - halfSize + x * cellSize;
                float worldZ = centerPos.z - halfSize + z * cellSize;

                int worldGridX = worldToGridIndex(worldX);
                int worldGridZ = worldToGridIndex(worldZ);

                auto key = std::make_pair(worldGridX, worldGridZ);
                auto it = worldDeformations.find(key);
                float height = (it != worldDeformations.end()) ? it->second : 0.0f;

                heights[z * gridSize + x] = height;

                // Offset slightly above ground plane to prevent z-fighting
                vertices[vertIdx] = worldX;
                vertices[vertIdx + 1] = height + 0.005f;
                vertices[vertIdx + 2] = worldZ;

                vertIdx += 9;
            }
        }

        // Second pass: calculate normals from height gradient
        vertIdx = 0;
        for (int z = 0; z < gridSize; z++) {
            for (int x = 0; x < gridSize; x++) {
                float hL = getHeight(x - 1, z);
                float hR = getHeight(x + 1, z);
                float hD = getHeight(x, z - 1);
                float hU = getHeight(x, z + 1);

                glm::vec3 normal;
                normal.x = (hL - hR) / (2.0f * cellSize);
                normal.y = 1.0f;
                normal.z = (hD - hU) / (2.0f * cellSize);
                normal = glm::normalize(normal);

                vertices[vertIdx + 3] = normal.x;
                vertices[vertIdx + 4] = normal.y;
                vertices[vertIdx + 5] = normal.z;

                float height = heights[z * gridSize + x];
                glm::vec3 color = (height < -0.01f) ? glm::vec3(1.0f, 0.0f, 0.0f) : glm::vec3(1.0f);
                vertices[vertIdx + 6] = color.r;
                vertices[vertIdx + 7] = color.g;
                vertices[vertIdx + 8] = color.b;

                vertIdx += 9;
            }
        }

        glBindBuffer(GL_ARRAY_BUFFER, vbo);
        glBufferSubData(GL_ARRAY_BUFFER, 0, vertices.size() * sizeof(float), vertices.data());
        needsRebuild = false;
    }

    float getHeight(int x, int z) const {
        x = glm::clamp(x, 0, gridSize - 1);
        z = glm::clamp(z, 0, gridSize - 1);
        return heights[z * gridSize + x];
    }

    void cleanup() {
        if (vao) { glDeleteVertexArrays(1, &vao); vao = 0; }
        if (vbo) { glDeleteBuffers(1, &vbo); vbo = 0; }
        if (ebo) { glDeleteBuffers(1, &ebo); ebo = 0; }
        initialized = false;
    }

    void deformAt(const glm::vec3& worldPos, float radius, float depth) {
        int radiusCells = static_cast<int>(std::ceil(radius / cellSize)) + 1;
        int centerGridX = worldToGridIndex(worldPos.x);
        int centerGridZ = worldToGridIndex(worldPos.z);

        for (int dz = -radiusCells; dz <= radiusCells; dz++) {
            for (int dx = -radiusCells; dx <= radiusCells; dx++) {
                int worldGridX = centerGridX + dx;
                int worldGridZ = centerGridZ + dz;

                float cellWorldX = worldGridX * cellSize + cellSize * 0.5f;
                float cellWorldZ = worldGridZ * cellSize + cellSize * 0.5f;

                float distX = cellWorldX - worldPos.x;
                float distZ = cellWorldZ - worldPos.z;
                float dist = std::sqrt(distX * distX + distZ * distZ);

                if (dist < radius) {
                    float factor = 1.0f - (dist / radius);
                    factor = factor * factor;
                    float deformHeight = -depth * factor;

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

    void resetHeights() {
        std::fill(heights.begin(), heights.end(), 0.0f);
        needsRebuild = true;
    }
};
