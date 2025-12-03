#pragma once

#include <glm/glm.hpp>
#include <cstdint>

namespace ecs {

enum class LightType : uint8_t {
    DIRECTIONAL,
    POINT,
    SPOTLIGHT
};

struct LightComponent {
    LightType type = LightType::POINT;

    // Light properties
    glm::vec3 color{1.0f};
    float intensity = 1.0f;

    // Direction (for directional and spotlight)
    // Note: Position comes from TransformComponent
    glm::vec3 direction{0.0f, -1.0f, 0.0f};

    // Spotlight specific
    float cutoff = glm::cos(glm::radians(12.5f));       // Inner cone angle (cosine)
    float outerCutoff = glm::cos(glm::radians(17.5f));  // Outer cone angle (cosine)

    // Attenuation (for point/spotlight)
    float constant = 1.0f;
    float linear = 0.09f;
    float quadratic = 0.032f;

    // State
    bool enabled = true;
    bool castShadows = false;

    // Shadow map settings (for future use)
    float shadowBias = 0.005f;
    float shadowNearPlane = 0.1f;
    float shadowFarPlane = 100.0f;

    LightComponent() = default;

    // Directional light constructor
    static LightComponent Directional(const glm::vec3& dir, const glm::vec3& col, float intensity = 1.0f) {
        LightComponent light;
        light.type = LightType::DIRECTIONAL;
        light.direction = glm::normalize(dir);
        light.color = col;
        light.intensity = intensity;
        return light;
    }

    // Point light constructor
    static LightComponent Point(const glm::vec3& col, float intensity = 1.0f, float range = 50.0f) {
        LightComponent light;
        light.type = LightType::POINT;
        light.color = col;
        light.intensity = intensity;
        // Calculate attenuation from range (approximation)
        light.linear = 4.5f / range;
        light.quadratic = 75.0f / (range * range);
        return light;
    }

    // Spotlight constructor
    static LightComponent Spotlight(const glm::vec3& dir, const glm::vec3& col,
                                    float intensity = 1.0f,
                                    float innerAngleDeg = 12.5f,
                                    float outerAngleDeg = 17.5f) {
        LightComponent light;
        light.type = LightType::SPOTLIGHT;
        light.direction = glm::normalize(dir);
        light.color = col;
        light.intensity = intensity;
        light.cutoff = glm::cos(glm::radians(innerAngleDeg));
        light.outerCutoff = glm::cos(glm::radians(outerAngleDeg));
        return light;
    }
};

} // namespace ecs
