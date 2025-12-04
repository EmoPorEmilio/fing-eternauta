#pragma once

#include "../ecs/System.h"
#include "../components/TransformComponent.h"
#include "../components/LightComponent.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>

namespace ecs {

// Maximum lights supported in shaders
constexpr int MAX_LIGHTS = 16;

// Light data for shader upload
struct LightData {
    glm::vec4 position;     // xyz = position, w = type (0=dir, 1=point, 2=spot)
    glm::vec4 direction;    // xyz = direction, w = enabled
    glm::vec4 color;        // xyz = color, w = intensity
    glm::vec4 attenuation;  // x = constant, y = linear, z = quadratic, w = unused
    glm::vec4 cutoff;       // x = inner cutoff, y = outer cutoff, z = unused, w = unused
};

// Manages lights and updates shader uniforms / UBOs
class LightSystem : public System {
public:
    LightSystem() = default;

    void init(Registry& registry) override {
        (void)registry;
        // Create UBO if needed
        if (m_useUBO && m_ubo == 0) {
            createUBO();
        }
    }

    void update(Registry& registry, float deltaTime) override {
        (void)deltaTime;

        // Gather light data from entities
        m_lightData.clear();
        m_lightCount = 0;

        registry.each<TransformComponent, LightComponent>(
            [this](TransformComponent& transform, LightComponent& light) {
                if (!light.enabled || m_lightCount >= MAX_LIGHTS) return;

                LightData data;

                // Position and type
                data.position = glm::vec4(
                    transform.position,
                    static_cast<float>(light.type)
                );

                // Direction and enabled
                data.direction = glm::vec4(light.direction, 1.0f);

                // Color and intensity
                data.color = glm::vec4(light.color, light.intensity);

                // Attenuation
                data.attenuation = glm::vec4(
                    light.constant,
                    light.linear,
                    light.quadratic,
                    0.0f
                );

                // Cutoff (for spotlights)
                data.cutoff = glm::vec4(
                    light.cutoff,
                    light.outerCutoff,
                    0.0f,
                    0.0f
                );

                m_lightData.push_back(data);
                m_lightCount++;
            });

        // Update UBO if using UBO mode
        if (m_useUBO && m_ubo != 0) {
            updateUBO();
        }
    }

    const char* name() const override { return "LightSystem"; }

    // Apply lights to shader using uniforms
    void applyToShader(GLuint shaderProgram, const glm::vec3& cameraPos) {
        glUseProgram(shaderProgram);

        // Set light count
        GLint countLoc = glGetUniformLocation(shaderProgram, "uLightCount");
        if (countLoc >= 0) {
            glUniform1i(countLoc, m_lightCount);
        }

        // Set camera position
        GLint viewPosLoc = glGetUniformLocation(shaderProgram, "uViewPos");
        if (viewPosLoc >= 0) {
            glUniform3fv(viewPosLoc, 1, glm::value_ptr(cameraPos));
        }

        // Set individual light uniforms
        for (int i = 0; i < m_lightCount; ++i) {
            const auto& light = m_lightData[i];
            std::string prefix = "uLights[" + std::to_string(i) + "].";

            setUniform(shaderProgram, prefix + "position", glm::vec3(light.position));
            setUniform(shaderProgram, prefix + "direction", glm::vec3(light.direction));
            setUniform(shaderProgram, prefix + "color", glm::vec3(light.color));
            setUniform(shaderProgram, prefix + "intensity", light.color.w);
            setUniform(shaderProgram, prefix + "type", static_cast<int>(light.position.w));
            setUniform(shaderProgram, prefix + "constant", light.attenuation.x);
            setUniform(shaderProgram, prefix + "linear", light.attenuation.y);
            setUniform(shaderProgram, prefix + "quadratic", light.attenuation.z);
            setUniform(shaderProgram, prefix + "cutoff", light.cutoff.x);
            setUniform(shaderProgram, prefix + "outerCutoff", light.cutoff.y);
        }
    }

    // Flashlight support (special handling)
    void setFlashlightEntity(Entity entity) { m_flashlightEntity = entity; }

    void updateFlashlight(Registry& registry, const glm::vec3& cameraPos, const glm::vec3& cameraFront) {
        if (!m_flashlightEntity.isValid()) return;

        auto* transform = registry.tryGet<TransformComponent>(m_flashlightEntity);
        auto* light = registry.tryGet<LightComponent>(m_flashlightEntity);

        if (transform && light) {
            transform->position = cameraPos;
            light->direction = cameraFront;
        }
    }

    // UBO support
    void setUseUBO(bool use) { m_useUBO = use; }
    GLuint getUBO() const { return m_ubo; }
    void bindUBO(GLuint bindingPoint) {
        if (m_ubo != 0) {
            glBindBufferBase(GL_UNIFORM_BUFFER, bindingPoint, m_ubo);
        }
    }

    // Getters
    int getLightCount() const { return m_lightCount; }
    const std::vector<LightData>& getLightData() const { return m_lightData; }

private:
    std::vector<LightData> m_lightData;
    int m_lightCount = 0;

    Entity m_flashlightEntity;

    bool m_useUBO = false;
    GLuint m_ubo = 0;
    static constexpr GLuint UBO_BINDING_POINT = 1;

    void createUBO() {
        glGenBuffers(1, &m_ubo);
        glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);
        glBufferData(GL_UNIFORM_BUFFER, sizeof(LightData) * MAX_LIGHTS + sizeof(int), nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void updateUBO() {
        glBindBuffer(GL_UNIFORM_BUFFER, m_ubo);

        // Upload light count
        glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(int), &m_lightCount);

        // Upload light data
        if (m_lightCount > 0) {
            glBufferSubData(GL_UNIFORM_BUFFER, sizeof(glm::vec4), // Align to vec4
                           sizeof(LightData) * m_lightCount, m_lightData.data());
        }

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    void setUniform(GLuint program, const std::string& name, float value) {
        GLint loc = glGetUniformLocation(program, name.c_str());
        if (loc >= 0) glUniform1f(loc, value);
    }

    void setUniform(GLuint program, const std::string& name, int value) {
        GLint loc = glGetUniformLocation(program, name.c_str());
        if (loc >= 0) glUniform1i(loc, value);
    }

    void setUniform(GLuint program, const std::string& name, const glm::vec3& value) {
        GLint loc = glGetUniformLocation(program, name.c_str());
        if (loc >= 0) glUniform3fv(loc, 1, glm::value_ptr(value));
    }
};

} // namespace ecs
