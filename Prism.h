#pragma once

#include <glm/glm.hpp>
#include <vector>

enum class LODLevel
{
    HIGH,   // Complex porous structure
    MEDIUM, // Simplified structure
    LOW     // Simple solid prism
};

class Prism
{
public:
    Prism();
    ~Prism();

    struct Vertex
    {
        glm::vec3 position;
        glm::vec3 normal;
        glm::vec2 uv;
    };

    void generateGeometry(LODLevel lod = LODLevel::HIGH);
    void generateRandomizedHighLODGeometry(int seed); // Public method for unique geometry
    const std::vector<Vertex> &getVertices() const { return vertices; }
    const std::vector<unsigned int> &getIndices() const { return indices; }

    // Get vertex count for performance monitoring
    size_t getVertexCount() const { return vertices.size(); }
    size_t getTriangleCount() const { return indices.size() / 3; }

private:
    std::vector<Vertex> vertices;
    std::vector<unsigned int> indices;

    void addVertex(const glm::vec3 &pos, const glm::vec3 &normal, const glm::vec2 &uv);
    void addTriangle(unsigned int i1, unsigned int i2, unsigned int i3);

    // LOD-specific geometry generation
    void generateHighLODGeometry();   // Complex porous structure
    void generateMediumLODGeometry(); // Simplified structure
    void generateLowLODGeometry();    // Simple solid prism

    // Helper functions for porous structure
    void addSmallPrism(const glm::vec3 &center, float size, bool isHole = false);
};
