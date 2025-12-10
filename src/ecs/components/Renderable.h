#pragma once
#include <glm/glm.hpp>

enum class ShaderType {
    Color,
    Model,
    Skinned,
    Terrain
};

struct Renderable {
    ShaderType shader = ShaderType::Model;
    glm::vec3 meshOffset{0.0f};  // Offset applied to mesh (e.g., to place feet at origin)
    bool triplanarMapping = false;  // Use world-space UV projection (for procedural geometry)
    float textureScale = 4.0f;      // World units per texture repeat (when triplanar enabled)
};
