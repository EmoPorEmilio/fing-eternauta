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
};
