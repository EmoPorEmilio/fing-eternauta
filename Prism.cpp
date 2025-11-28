#include "Prism.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <random>
#include <cmath>

Prism::Prism()
{
    generateGeometry(LODLevel::HIGH);
}

Prism::~Prism()
{
}

void Prism::generateGeometry(LODLevel lod)
{
    vertices.clear();
    indices.clear();

    switch (lod)
    {
    case LODLevel::HIGH:
        generateHighLODGeometry();
        break;
    case LODLevel::MEDIUM:
        generateMediumLODGeometry();
        break;
    case LODLevel::LOW:
        generateLowLODGeometry();
        break;
    }
}

void Prism::generateHighLODGeometry()
{
    // Create an INSANELY complex structure - MASSIVE geometry for extreme LOD testing
    // This will be extremely expensive to render, making LOD differences VERY obvious

    // Base structure with DRAMATIC randomization to prevent GPU batching
    float baseVariation = 0.3f * (rand() % 100) / 100.0f; // 0.0 to 0.3 variation (much larger!)
    addSmallPrism(glm::vec3(0.0f, 0.0f, 0.0f), 0.6f + baseVariation, false);

    // Primary structural elements (8 corners of a cube) with DRAMATIC randomization
    float cornerVariation = 0.2f * (rand() % 100) / 100.0f; // 0.0 to 0.2 variation (much larger!)
    float posVariation = 0.1f * (rand() % 100) / 100.0f;    // 0.0 to 0.1 position variation
    addSmallPrism(glm::vec3(-0.4f + posVariation, 0.4f + posVariation, -0.4f + posVariation), 0.2f + cornerVariation, false);
    addSmallPrism(glm::vec3(0.4f + posVariation, 0.4f + posVariation, -0.4f + posVariation), 0.2f + cornerVariation, false);
    addSmallPrism(glm::vec3(-0.4f + posVariation, -0.4f + posVariation, -0.4f + posVariation), 0.2f + cornerVariation, false);
    addSmallPrism(glm::vec3(0.4f + posVariation, -0.4f + posVariation, -0.4f + posVariation), 0.2f + cornerVariation, false);
    addSmallPrism(glm::vec3(-0.4f + posVariation, 0.4f + posVariation, 0.4f + posVariation), 0.2f + cornerVariation, false);
    addSmallPrism(glm::vec3(0.4f + posVariation, 0.4f + posVariation, 0.4f + posVariation), 0.2f + cornerVariation, false);
    addSmallPrism(glm::vec3(-0.4f + posVariation, -0.4f + posVariation, 0.4f + posVariation), 0.2f + cornerVariation, false);
    addSmallPrism(glm::vec3(0.4f + posVariation, -0.4f + posVariation, 0.4f + posVariation), 0.2f + cornerVariation, false);

    // Mid-edge elements (12 edges of a cube) with DRAMATIC randomization
    float edgeVariation = 0.15f * (rand() % 100) / 100.0f; // 0.0 to 0.15 variation (much larger!)
    addSmallPrism(glm::vec3(0.0f, 0.4f, -0.4f), 0.15f + edgeVariation, false);
    addSmallPrism(glm::vec3(0.0f, -0.4f, -0.4f), 0.15f + edgeVariation, false);
    addSmallPrism(glm::vec3(-0.4f, 0.0f, -0.4f), 0.15f + edgeVariation, false);
    addSmallPrism(glm::vec3(0.4f, 0.0f, -0.4f), 0.15f + edgeVariation, false);
    addSmallPrism(glm::vec3(0.0f, 0.4f, 0.4f), 0.15f + edgeVariation, false);
    addSmallPrism(glm::vec3(0.0f, -0.4f, 0.4f), 0.15f + edgeVariation, false);
    addSmallPrism(glm::vec3(-0.4f, 0.0f, 0.4f), 0.15f + edgeVariation, false);
    addSmallPrism(glm::vec3(0.4f, 0.0f, 0.4f), 0.15f + edgeVariation, false);
    addSmallPrism(glm::vec3(-0.4f, 0.4f, 0.0f), 0.15f + edgeVariation, false);
    addSmallPrism(glm::vec3(0.4f, 0.4f, 0.0f), 0.15f + edgeVariation, false);
    addSmallPrism(glm::vec3(-0.4f, -0.4f, 0.0f), 0.15f + edgeVariation, false);
    addSmallPrism(glm::vec3(0.4f, -0.4f, 0.0f), 0.15f + edgeVariation, false);

    // Center face elements (6 faces of a cube)
    addSmallPrism(glm::vec3(0.0f, 0.0f, -0.4f), 0.25f, false);
    addSmallPrism(glm::vec3(0.0f, 0.0f, 0.4f), 0.25f, false);
    addSmallPrism(glm::vec3(-0.4f, 0.0f, 0.0f), 0.25f, false);
    addSmallPrism(glm::vec3(0.4f, 0.0f, 0.0f), 0.25f, false);
    addSmallPrism(glm::vec3(0.0f, 0.4f, 0.0f), 0.25f, false);
    addSmallPrism(glm::vec3(0.0f, -0.4f, 0.0f), 0.25f, false);

    // Additional detail elements (many small pieces for extra complexity)
    addSmallPrism(glm::vec3(-0.2f, 0.2f, -0.2f), 0.1f, false);
    addSmallPrism(glm::vec3(0.2f, 0.2f, -0.2f), 0.1f, false);
    addSmallPrism(glm::vec3(-0.2f, -0.2f, -0.2f), 0.1f, false);
    addSmallPrism(glm::vec3(0.2f, -0.2f, -0.2f), 0.1f, false);
    addSmallPrism(glm::vec3(-0.2f, 0.2f, 0.2f), 0.1f, false);
    addSmallPrism(glm::vec3(0.2f, 0.2f, 0.2f), 0.1f, false);
    addSmallPrism(glm::vec3(-0.2f, -0.2f, 0.2f), 0.1f, false);
    addSmallPrism(glm::vec3(0.2f, -0.2f, 0.2f), 0.1f, false);

    // Even more detail elements
    addSmallPrism(glm::vec3(-0.1f, 0.1f, -0.1f), 0.08f, false);
    addSmallPrism(glm::vec3(0.1f, 0.1f, -0.1f), 0.08f, false);
    addSmallPrism(glm::vec3(-0.1f, -0.1f, -0.1f), 0.08f, false);
    addSmallPrism(glm::vec3(0.1f, -0.1f, -0.1f), 0.08f, false);
    addSmallPrism(glm::vec3(-0.1f, 0.1f, 0.1f), 0.08f, false);
    addSmallPrism(glm::vec3(0.1f, 0.1f, 0.1f), 0.08f, false);
    addSmallPrism(glm::vec3(-0.1f, -0.1f, 0.1f), 0.08f, false);
    addSmallPrism(glm::vec3(0.1f, -0.1f, 0.1f), 0.08f, false);

    // EXTRA COMPLEXITY: Add even more tiny detail elements
    // Micro details for maximum geometry complexity
    addSmallPrism(glm::vec3(-0.05f, 0.05f, -0.05f), 0.04f, false);
    addSmallPrism(glm::vec3(0.05f, 0.05f, -0.05f), 0.04f, false);
    addSmallPrism(glm::vec3(-0.05f, -0.05f, -0.05f), 0.04f, false);
    addSmallPrism(glm::vec3(0.05f, -0.05f, -0.05f), 0.04f, false);
    addSmallPrism(glm::vec3(-0.05f, 0.05f, 0.05f), 0.04f, false);
    addSmallPrism(glm::vec3(0.05f, 0.05f, 0.05f), 0.04f, false);
    addSmallPrism(glm::vec3(-0.05f, -0.05f, 0.05f), 0.04f, false);
    addSmallPrism(glm::vec3(0.05f, -0.05f, 0.05f), 0.04f, false);

    // Even more micro details
    addSmallPrism(glm::vec3(-0.3f, 0.3f, -0.3f), 0.06f, false);
    addSmallPrism(glm::vec3(0.3f, 0.3f, -0.3f), 0.06f, false);
    addSmallPrism(glm::vec3(-0.3f, -0.3f, -0.3f), 0.06f, false);
    addSmallPrism(glm::vec3(0.3f, -0.3f, -0.3f), 0.06f, false);
    addSmallPrism(glm::vec3(-0.3f, 0.3f, 0.3f), 0.06f, false);
    addSmallPrism(glm::vec3(0.3f, 0.3f, 0.3f), 0.06f, false);
    addSmallPrism(glm::vec3(-0.3f, -0.3f, 0.3f), 0.06f, false);
    addSmallPrism(glm::vec3(0.3f, -0.3f, 0.3f), 0.06f, false);

    // Total: 1 base + 8 corners + 12 edges + 6 faces + 8 medium + 8 small + 8 micro + 8 tiny = 59 prisms!
    // This is INSANELY complex and will be VERY expensive to render!
}

void Prism::generateRandomizedHighLODGeometry(int seed)
{
    // Create a UNIQUE complex structure for each object to prevent GPU batching optimization
    // Each object will have different geometry, making LOD performance differences obvious

    // Use seed to create deterministic but unique geometry
    srand(seed);

    // Base structure with random size
    float baseSize = 0.5f + (rand() % 100) / 200.0f; // 0.5 to 1.0
    addSmallPrism(glm::vec3(0.0f, 0.0f, 0.0f), baseSize, false);

    // Random number of structural elements (8-15)
    int numCorners = 8 + (rand() % 8);
    for (int i = 0; i < numCorners; i++)
    {
        float x = -0.4f + (rand() % 800) / 1000.0f;   // -0.4 to 0.4
        float y = -0.4f + (rand() % 800) / 1000.0f;   // -0.4 to 0.4
        float z = -0.4f + (rand() % 800) / 1000.0f;   // -0.4 to 0.4
        float size = 0.1f + (rand() % 200) / 1000.0f; // 0.1 to 0.3
        addSmallPrism(glm::vec3(x, y, z), size, false);
    }

    // Random number of edge elements (10-20)
    int numEdges = 10 + (rand() % 11);
    for (int i = 0; i < numEdges; i++)
    {
        float x = -0.4f + (rand() % 800) / 1000.0f;
        float y = -0.4f + (rand() % 800) / 1000.0f;
        float z = -0.4f + (rand() % 800) / 1000.0f;
        float size = 0.08f + (rand() % 150) / 1000.0f; // 0.08 to 0.23
        addSmallPrism(glm::vec3(x, y, z), size, false);
    }

    // Random number of face elements (4-8)
    int numFaces = 4 + (rand() % 5);
    for (int i = 0; i < numFaces; i++)
    {
        float x = -0.4f + (rand() % 800) / 1000.0f;
        float y = -0.4f + (rand() % 800) / 1000.0f;
        float z = -0.4f + (rand() % 800) / 1000.0f;
        float size = 0.15f + (rand() % 200) / 1000.0f; // 0.15 to 0.35
        addSmallPrism(glm::vec3(x, y, z), size, false);
    }

    // Random number of detail elements (15-30)
    int numDetails = 15 + (rand() % 16);
    for (int i = 0; i < numDetails; i++)
    {
        float x = -0.3f + (rand() % 600) / 1000.0f;
        float y = -0.3f + (rand() % 600) / 1000.0f;
        float z = -0.3f + (rand() % 600) / 1000.0f;
        float size = 0.04f + (rand() % 120) / 1000.0f; // 0.04 to 0.16
        addSmallPrism(glm::vec3(x, y, z), size, false);
    }

    // Random number of micro elements (10-25)
    int numMicro = 10 + (rand() % 16);
    for (int i = 0; i < numMicro; i++)
    {
        float x = -0.2f + (rand() % 400) / 1000.0f;
        float y = -0.2f + (rand() % 400) / 1000.0f;
        float z = -0.2f + (rand() % 400) / 1000.0f;
        float size = 0.02f + (rand() % 80) / 1000.0f; // 0.02 to 0.10
        addSmallPrism(glm::vec3(x, y, z), size, false);
    }

    // Total: Each object will have 47-98 unique prisms!
    // This prevents GPU batching optimization and makes LOD performance differences obvious!
}

void Prism::generateMediumLODGeometry()
{
    // Much simpler structure - only 4 elements for dramatic LOD contrast
    addSmallPrism(glm::vec3(0.0f, 0.0f, 0.0f), 0.8f, false);
    addSmallPrism(glm::vec3(-0.3f, 0.3f, 0.0f), 0.4f, false);
    addSmallPrism(glm::vec3(0.3f, -0.3f, 0.0f), 0.4f, false);
    addSmallPrism(glm::vec3(0.0f, 0.0f, -0.3f), 0.4f, false);
    // Total: Only 4 prisms vs 43 in high LOD!
}

void Prism::generateLowLODGeometry()
{
    // Simple solid prism - original geometry
    // Base vertices (Y=0)
    addVertex(glm::vec3(-0.5f, 0.0f, -0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f)); // 0
    addVertex(glm::vec3(0.5f, 0.0f, -0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 0.0f));  // 1
    addVertex(glm::vec3(0.5f, 0.0f, 0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 1.0f));   // 2
    addVertex(glm::vec3(-0.5f, 0.0f, 0.5f), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 1.0f));  // 3

    // Top vertices (Y=1)
    addVertex(glm::vec3(-0.5f, 1.0f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f)); // 4
    addVertex(glm::vec3(0.5f, 1.0f, -0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f));  // 5
    addVertex(glm::vec3(0.5f, 1.0f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f));   // 6
    addVertex(glm::vec3(-0.5f, 1.0f, 0.5f), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f));  // 7

    // Base face (bottom)
    addTriangle(0, 2, 1);
    addTriangle(0, 3, 2);

    // Top face
    addTriangle(4, 5, 6);
    addTriangle(4, 6, 7);

    // Side faces
    // Front face (Z = -0.5)
    addVertex(glm::vec3(-0.5f, 0.0f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 0.0f)); // 8
    addVertex(glm::vec3(0.5f, 0.0f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 0.0f));  // 9
    addVertex(glm::vec3(0.5f, 1.0f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(1.0f, 1.0f));  // 10
    addVertex(glm::vec3(-0.5f, 1.0f, -0.5f), glm::vec3(0.0f, 0.0f, -1.0f), glm::vec2(0.0f, 1.0f)); // 11
    addTriangle(8, 9, 10);
    addTriangle(8, 10, 11);

    // Back face (Z = 0.5)
    addVertex(glm::vec3(0.5f, 0.0f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 0.0f));  // 12
    addVertex(glm::vec3(-0.5f, 0.0f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 0.0f)); // 13
    addVertex(glm::vec3(-0.5f, 1.0f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(1.0f, 1.0f)); // 14
    addVertex(glm::vec3(0.5f, 1.0f, 0.5f), glm::vec3(0.0f, 0.0f, 1.0f), glm::vec2(0.0f, 1.0f));  // 15
    addTriangle(12, 13, 14);
    addTriangle(12, 14, 15);

    // Left face (X = -0.5)
    addVertex(glm::vec3(-0.5f, 0.0f, 0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f));  // 16
    addVertex(glm::vec3(-0.5f, 0.0f, -0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f)); // 17
    addVertex(glm::vec3(-0.5f, 1.0f, -0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f)); // 18
    addVertex(glm::vec3(-0.5f, 1.0f, 0.5f), glm::vec3(-1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f));  // 19
    addTriangle(16, 17, 18);
    addTriangle(16, 18, 19);

    // Right face (X = 0.5)
    addVertex(glm::vec3(0.5f, 0.0f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 0.0f)); // 20
    addVertex(glm::vec3(0.5f, 0.0f, 0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 0.0f));  // 21
    addVertex(glm::vec3(0.5f, 1.0f, 0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(1.0f, 1.0f));  // 22
    addVertex(glm::vec3(0.5f, 1.0f, -0.5f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec2(0.0f, 1.0f)); // 23
    addTriangle(20, 21, 22);
    addTriangle(20, 22, 23);
}

void Prism::addSmallPrism(const glm::vec3 &center, float size, bool isHole)
{
    float halfSize = size * 0.5f;

    // Create a small prism at the given center
    // Base vertices
    addVertex(center + glm::vec3(-halfSize, -halfSize, -halfSize), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 0.0f));
    addVertex(center + glm::vec3(halfSize, -halfSize, -halfSize), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 0.0f));
    addVertex(center + glm::vec3(halfSize, -halfSize, halfSize), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(1.0f, 1.0f));
    addVertex(center + glm::vec3(-halfSize, -halfSize, halfSize), glm::vec3(0.0f, -1.0f, 0.0f), glm::vec2(0.0f, 1.0f));

    // Top vertices
    addVertex(center + glm::vec3(-halfSize, halfSize, -halfSize), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 0.0f));
    addVertex(center + glm::vec3(halfSize, halfSize, -halfSize), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 0.0f));
    addVertex(center + glm::vec3(halfSize, halfSize, halfSize), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(1.0f, 1.0f));
    addVertex(center + glm::vec3(-halfSize, halfSize, halfSize), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec2(0.0f, 1.0f));

    size_t baseIndex = vertices.size() - 8;

    if (!isHole)
    {
        // Add faces for solid prism
        // Base
        addTriangle(static_cast<unsigned int>(baseIndex), static_cast<unsigned int>(baseIndex + 2), static_cast<unsigned int>(baseIndex + 1));
        addTriangle(static_cast<unsigned int>(baseIndex), static_cast<unsigned int>(baseIndex + 3), static_cast<unsigned int>(baseIndex + 2));

        // Top
        addTriangle(static_cast<unsigned int>(baseIndex + 4), static_cast<unsigned int>(baseIndex + 5), static_cast<unsigned int>(baseIndex + 6));
        addTriangle(static_cast<unsigned int>(baseIndex + 4), static_cast<unsigned int>(baseIndex + 6), static_cast<unsigned int>(baseIndex + 7));

        // Sides
        addTriangle(static_cast<unsigned int>(baseIndex), static_cast<unsigned int>(baseIndex + 1), static_cast<unsigned int>(baseIndex + 5));
        addTriangle(static_cast<unsigned int>(baseIndex), static_cast<unsigned int>(baseIndex + 5), static_cast<unsigned int>(baseIndex + 4));

        addTriangle(static_cast<unsigned int>(baseIndex + 1), static_cast<unsigned int>(baseIndex + 2), static_cast<unsigned int>(baseIndex + 6));
        addTriangle(static_cast<unsigned int>(baseIndex + 1), static_cast<unsigned int>(baseIndex + 6), static_cast<unsigned int>(baseIndex + 5));

        addTriangle(static_cast<unsigned int>(baseIndex + 2), static_cast<unsigned int>(baseIndex + 3), static_cast<unsigned int>(baseIndex + 7));
        addTriangle(static_cast<unsigned int>(baseIndex + 2), static_cast<unsigned int>(baseIndex + 7), static_cast<unsigned int>(baseIndex + 6));

        addTriangle(static_cast<unsigned int>(baseIndex + 3), static_cast<unsigned int>(baseIndex), static_cast<unsigned int>(baseIndex + 4));
        addTriangle(static_cast<unsigned int>(baseIndex + 3), static_cast<unsigned int>(baseIndex + 4), static_cast<unsigned int>(baseIndex + 7));
    }
}

void Prism::addVertex(const glm::vec3 &pos, const glm::vec3 &normal, const glm::vec2 &uv)
{
    vertices.push_back({pos, normal, uv});
}

void Prism::addTriangle(unsigned int i1, unsigned int i2, unsigned int i3)
{
    indices.push_back(i1);
    indices.push_back(i2);
    indices.push_back(i3);
}