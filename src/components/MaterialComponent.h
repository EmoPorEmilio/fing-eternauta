#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>

namespace ecs {

struct MaterialComponent {
    // PBR base properties
    glm::vec4 baseColor{1.0f, 1.0f, 1.0f, 1.0f};
    float metallic = 0.0f;
    float roughness = 0.5f;
    float occlusion = 1.0f;
    float normalScale = 1.0f;

    // Emission
    glm::vec3 emissive{0.0f};
    float emissiveStrength = 1.0f;

    // Texture handles
    GLuint albedoTexture = 0;
    GLuint metallicRoughnessTexture = 0;
    GLuint normalTexture = 0;
    GLuint occlusionTexture = 0;
    GLuint emissiveTexture = 0;

    // Texture flags
    bool hasAlbedoTexture = false;
    bool hasMetallicRoughnessTexture = false;
    bool hasNormalTexture = false;
    bool hasOcclusionTexture = false;
    bool hasEmissiveTexture = false;

    // Rendering flags
    bool doubleSided = false;
    bool transparent = false;
    float alphaCutoff = 0.5f;

    MaterialComponent() = default;

    // Simple constructor for solid color
    explicit MaterialComponent(const glm::vec4& color)
        : baseColor(color) {}

    // PBR constructor
    MaterialComponent(const glm::vec4& color, float metal, float rough)
        : baseColor(color), metallic(metal), roughness(rough) {}
};

} // namespace ecs
