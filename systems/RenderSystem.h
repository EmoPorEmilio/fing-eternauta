#pragma once

#include "../ecs/System.h"
#include "../components/TransformComponent.h"
#include "../components/RenderableComponent.h"
#include "../components/MaterialComponent.h"
#include "../components/LODComponent.h"
#include "../components/BatchGroupComponent.h"
#include "../Prism.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <vector>
#include <unordered_map>

namespace ecs {

// Fog settings (shared across all renderables)
struct FogSettings {
    bool enabled = true;
    glm::vec3 color{0.0f, 0.0f, 0.0f};
    float density = 0.01f;
    float desaturationStrength = 0.8f;
    float absorptionDensity = 0.02f;
    float absorptionStrength = 0.8f;
};

// Instance batch for GPU instancing
struct InstanceBatch {
    GLuint vao = 0;
    GLuint shader = 0;
    LODLevel lod = LODLevel::HIGH;
    std::vector<glm::mat4> matrices;
};

// Unified render system - handles all renderable types
class RenderSystem : public System {
public:
    RenderSystem() = default;

    void init(Registry& registry) override {
        (void)registry;
    }

    void update(Registry& registry, float deltaTime) override {
        (void)deltaTime;
        // RenderSystem doesn't update in the normal update loop
        // Call render() explicitly from the main loop
    }

    const char* name() const override { return "RenderSystem"; }

    // Main render method - call this from the render loop
    void render(Registry& registry,
                const glm::mat4& view,
                const glm::mat4& projection,
                const glm::vec3& cameraPos,
                const glm::vec3& cameraFront) {

        // Clear batches
        m_instanceBatches.clear();

        // Gather visible renderables by type
        registry.each<TransformComponent, RenderableComponent>(
            [this, &registry](Entity entity, TransformComponent& transform, RenderableComponent& renderable) {
                if (!renderable.visible) return;

                switch (renderable.type) {
                    case RenderableType::INSTANCED_PRISM:
                        gatherInstancedPrism(registry, entity, transform, renderable);
                        break;

                    case RenderableType::GLTF_MODEL:
                        // Models rendered separately via ModelManager for now
                        break;

                    case RenderableType::PARTICLE:
                        // Particles rendered separately via SnowSystem for now
                        break;

                    case RenderableType::FLOOR:
                        // Floor rendered separately for now
                        break;

                    default:
                        break;
                }
            });

        // Render instanced batches
        renderInstancedBatches(view, projection, cameraPos, cameraFront);
    }

    // Fog settings
    void setFogSettings(const FogSettings& settings) { m_fog = settings; }
    FogSettings& getFogSettings() { return m_fog; }
    const FogSettings& getFogSettings() const { return m_fog; }

    // Apply fog uniforms to shader
    void applyFogUniforms(GLuint shaderProgram) const {
        glUniform1i(glGetUniformLocation(shaderProgram, "uFogEnabled"), m_fog.enabled ? 1 : 0);
        glUniform3fv(glGetUniformLocation(shaderProgram, "uFogColor"), 1, glm::value_ptr(m_fog.color));
        glUniform1f(glGetUniformLocation(shaderProgram, "uFogDensity"), m_fog.density);
        glUniform1f(glGetUniformLocation(shaderProgram, "uFogDesaturationStrength"), m_fog.desaturationStrength);
        glUniform1f(glGetUniformLocation(shaderProgram, "uFogAbsorptionDensity"), m_fog.absorptionDensity);
        glUniform1f(glGetUniformLocation(shaderProgram, "uFogAbsorptionStrength"), m_fog.absorptionStrength);
    }

    // Set shared resources (called during initialization)
    void setPrismVAOs(GLuint highVAO, GLuint medVAO, GLuint lowVAO) {
        m_prismVAO[0] = highVAO;
        m_prismVAO[1] = medVAO;
        m_prismVAO[2] = lowVAO;
    }

    void setPrismInstanceVBOs(GLuint highVBO, GLuint medVBO, GLuint lowVBO) {
        m_prismInstanceVBO[0] = highVBO;
        m_prismInstanceVBO[1] = medVBO;
        m_prismInstanceVBO[2] = lowVBO;
    }

    void setPrismIndexCounts(size_t high, size_t med, size_t low) {
        m_prismIndexCount[0] = high;
        m_prismIndexCount[1] = med;
        m_prismIndexCount[2] = low;
    }

    void setPrismShader(GLuint shader) { m_prismShader = shader; }

private:
    FogSettings m_fog;

    // Instance batches keyed by (shader, VAO, LOD)
    std::vector<InstanceBatch> m_instanceBatches;

    // Prism rendering resources (set externally)
    GLuint m_prismVAO[3] = {0, 0, 0};
    GLuint m_prismInstanceVBO[3] = {0, 0, 0};
    size_t m_prismIndexCount[3] = {0, 0, 0};
    GLuint m_prismShader = 0;

    // Per-LOD instance matrices
    std::vector<glm::mat4> m_highLODMatrices;
    std::vector<glm::mat4> m_medLODMatrices;
    std::vector<glm::mat4> m_lowLODMatrices;

    void gatherInstancedPrism(Registry& registry, Entity entity,
                              TransformComponent& transform, RenderableComponent& renderable) {
        (void)renderable;

        LODLevel lod = LODLevel::HIGH;

        // Get LOD level if available
        if (auto* lodComp = registry.tryGet<LODComponent>(entity)) {
            lod = lodComp->currentLevel;
        }

        // Add to appropriate batch
        switch (lod) {
            case LODLevel::HIGH:
                m_highLODMatrices.push_back(transform.modelMatrix);
                break;
            case LODLevel::MEDIUM:
                m_medLODMatrices.push_back(transform.modelMatrix);
                break;
            case LODLevel::LOW:
                m_lowLODMatrices.push_back(transform.modelMatrix);
                break;
        }
    }

    void renderInstancedBatches(const glm::mat4& view, const glm::mat4& projection,
                                const glm::vec3& cameraPos, const glm::vec3& cameraFront) {
        if (m_prismShader == 0) return;

        glUseProgram(m_prismShader);

        // Set common uniforms
        glUniformMatrix4fv(glGetUniformLocation(m_prismShader, "uView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniformMatrix4fv(glGetUniformLocation(m_prismShader, "uProj"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniform3fv(glGetUniformLocation(m_prismShader, "uViewPos"), 1, glm::value_ptr(cameraPos));

        // Apply fog
        applyFogUniforms(m_prismShader);

        // Render each LOD level
        renderLODLevel(LODLevel::HIGH, m_highLODMatrices);
        renderLODLevel(LODLevel::MEDIUM, m_medLODMatrices);
        renderLODLevel(LODLevel::LOW, m_lowLODMatrices);

        // Clear for next frame
        m_highLODMatrices.clear();
        m_medLODMatrices.clear();
        m_lowLODMatrices.clear();
    }

    void renderLODLevel(LODLevel lod, const std::vector<glm::mat4>& matrices) {
        if (matrices.empty()) return;

        int lodIndex = static_cast<int>(lod);
        GLuint vao = m_prismVAO[lodIndex];
        GLuint instanceVBO = m_prismInstanceVBO[lodIndex];
        size_t indexCount = m_prismIndexCount[lodIndex];

        if (vao == 0 || instanceVBO == 0 || indexCount == 0) return;

        glBindVertexArray(vao);

        // Upload instance matrices
        glBindBuffer(GL_ARRAY_BUFFER, instanceVBO);
        glBufferData(GL_ARRAY_BUFFER, matrices.size() * sizeof(glm::mat4), matrices.data(), GL_DYNAMIC_DRAW);

        // Draw instanced
        glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(indexCount),
                               GL_UNSIGNED_INT, nullptr, static_cast<GLsizei>(matrices.size()));

        glBindVertexArray(0);
    }
};

} // namespace ecs
