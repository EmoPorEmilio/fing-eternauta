/**
 * @file ModelManager.h
 * @brief GLTF model loading and PBR rendering
 *
 * ModelManager handles loading GLTF models via tinygltf and rendering them
 * with PBR (Physically Based Rendering) materials. Supports skeletal animation,
 * instancing, and per-instance visibility control.
 *
 * Model Loading:
 *   - loadModel(filepath, name) - Loads GLTF/GLB file
 *   - Models stored as unique_ptr<GLTFModel> in m_models vector
 *   - Supports embedded textures and external texture files
 *
 * Instancing:
 *   - addModelInstance(name, transform) - Creates renderable instance
 *   - Each instance is an ECS entity with TransformComponent + ModelRefComponent
 *   - Per-instance visibility and transform control
 *
 * PBR Shader (pbr_model.vert/frag):
 *   - GGX normal distribution + Schlick-GGX geometry
 *   - Fresnel approximation for specular reflection
 *   - Supports baseColor, metallic, roughness, normal, occlusion textures
 *   - Integrates with flashlight UBO and fog system
 *
 * ECS Integration:
 *   - Entities created with TransformComponent, RenderableComponent, ModelRefComponent
 *   - ModelRefComponent stores pointer to GLTFModel (fragile if model removed)
 *   - Consider using model index instead for safety
 *
 * Known Issues:
 *   - No LOD system for models (renders full detail at all distances)
 *   - Per-instance draw calls (no GPU instancing of model copies)
 *
 * @see GLTFModel, pbr_model.vert, pbr_model.frag
 */
#pragma once

#include "GLTFModel.h"
#include "Shader.h"
#include "LightManager.h"
#include <vector>
#include <memory>

// ECS includes
#include "ECSWorld.h"
#include "components/TransformComponent.h"
#include "components/RenderableComponent.h"
#include "components/MaterialComponent.h"

class ModelManager
{
public:
    ModelManager();
    ~ModelManager();

    // Initialization
    bool initialize();
    void cleanup();

    // Model management
    bool loadModel(const std::string &filepath, const std::string &name = "");
    void removeModel(const std::string &name);
    GLTFModel *getModel(const std::string &name);

    // Instance management
    int addModelInstance(const std::string &modelName, const glm::mat4 &transform = glm::mat4(1.0f));
    void removeModelInstance(int instanceId);
    void setInstanceTransform(int instanceId, const glm::mat4 &transform);
    void setInstanceVisibility(int instanceId, bool visible);

    // Rendering
    void render(const glm::mat4 &view, const glm::mat4 &projection,
                const glm::vec3 &cameraPos, const glm::vec3 &lightDir,
                const glm::vec3 &lightColor, const LightManager &lightManager);

    // Getters
    size_t getModelCount() const { return m_models.size(); }
    size_t getInstanceCount() const { return m_instanceEntities.size(); }
    bool isInitialized() const { return m_isInitialized; }

    // Performance stats
    size_t getTotalVertexCount() const;
    size_t getTotalTriangleCount() const;
    void printStats() const;

    // Fog configuration (will be moved to Config system in Phase 3)
    void setFogEnabled(bool enabled) { m_fogEnabled = enabled; }
    void setFogColor(const glm::vec3 &color) { m_fogColor = color; }
    void setFogDensity(float density) { m_fogDensity = density; }
    void setFogDesaturationStrength(float strength) { m_fogDesaturationStrength = strength; }
    void setFogAbsorption(float density, float strength)
    {
        m_fogAbsorptionDensity = density;
        m_fogAbsorptionStrength = strength;
    }

    // Custom component for linking entity to GLTFModel pointer
    struct ModelRefComponent {
        GLTFModel* model = nullptr;
    };

private:
    std::vector<std::pair<std::string, std::unique_ptr<GLTFModel>>> m_models;
    Shader m_pbrShader;
    bool m_isInitialized;

    // Fog parameters (will be moved to Config in Phase 3)
    bool m_fogEnabled;
    glm::vec3 m_fogColor;
    float m_fogDensity;
    float m_fogDesaturationStrength;
    float m_fogAbsorptionDensity;
    float m_fogAbsorptionStrength;

    // Helper functions
    bool setupPBRShader();
    int findModelIndex(const std::string &name);

    // Entity tracking
    std::vector<ecs::Entity> m_instanceEntities;

    // ECS helper methods
    ecs::Entity createModelInstanceEntity(GLTFModel* model, const glm::mat4& transform);

    // Cached uniform locations (avoids glGetUniformLocation every frame)
    struct UniformCache {
        GLint cameraPos = -1;
        GLint lightDir = -1;
        GLint lightColor = -1;
        GLint exposure = -1;
        GLint flashlightOn = -1;
        GLint flashlightPos = -1;
        GLint flashlightDir = -1;
        GLint flashlightCutoff = -1;
        GLint flashlightBrightness = -1;
        GLint flashlightColor = -1;
        GLint debugNormals = -1;
        GLint fogEnabled = -1;
        GLint fogColor = -1;
        GLint fogDensity = -1;
        GLint fogDesaturationStrength = -1;
        GLint fogAbsorptionDensity = -1;
        GLint fogAbsorptionStrength = -1;
        GLint backgroundColor = -1;
    };
    UniformCache m_uniforms;
    void cacheUniformLocations();
};
