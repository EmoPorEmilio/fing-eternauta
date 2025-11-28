#pragma once

#include "GLTFModel.h"
#include "Shader.h"
#include "LightManager.h"
#include <vector>
#include <memory>

struct ModelInstance
{
    GLTFModel *model;
    glm::mat4 transform;
    bool isVisible;

    ModelInstance(GLTFModel *model, const glm::mat4 &transform = glm::mat4(1.0f))
        : model(model), transform(transform), isVisible(true) {}
};

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
    size_t getInstanceCount() const { return m_instances.size(); }
    bool isInitialized() const { return m_isInitialized; }

    // Performance stats
    size_t getTotalVertexCount() const;
    size_t getTotalTriangleCount() const;
    void printStats() const;

    // Fog configuration
    void setFogEnabled(bool enabled) { m_fogEnabled = enabled; }
    void setFogColor(const glm::vec3 &color) { m_fogColor = color; }
    void setFogDensity(float density) { m_fogDensity = density; }
    void setFogDesaturationStrength(float strength) { m_fogDesaturationStrength = strength; }
    void setFogAbsorption(float density, float strength)
    {
        m_fogAbsorptionDensity = density;
        m_fogAbsorptionStrength = strength;
    }

private:
    std::vector<std::pair<std::string, std::unique_ptr<GLTFModel>>> m_models;
    std::vector<ModelInstance> m_instances;
    Shader m_pbrShader;
    bool m_isInitialized;

    // Fog parameters
    bool m_fogEnabled;
    glm::vec3 m_fogColor;
    float m_fogDensity;
    float m_fogDesaturationStrength;
    float m_fogAbsorptionDensity;
    float m_fogAbsorptionStrength;

    // Helper functions
    bool setupPBRShader();
    int findModelIndex(const std::string &name);
};
