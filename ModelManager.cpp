#include "ModelManager.h"
#include <iostream>

ModelManager::ModelManager()
    : m_isInitialized(false), m_fogEnabled(true), m_fogColor(0.0f, 0.0f, 0.0f),
      m_fogDensity(0.01f), m_fogDesaturationStrength(1.0f),
      m_fogAbsorptionDensity(0.02f), m_fogAbsorptionStrength(0.8f)
{
}

ModelManager::~ModelManager()
{
    cleanup();
}

bool ModelManager::initialize()
{
    if (m_isInitialized)
    {
        return true;
    }

    std::cout << "[ModelManager] Initializing..." << std::endl;

    if (!setupPBRShader())
    {
        std::cerr << "[ModelManager] Failed to setup PBR shader" << std::endl;
        return false;
    }

    m_instanceEntities.clear();

    m_isInitialized = true;
    std::cout << "[ModelManager] Initialized successfully!" << std::endl;
    return true;
}

void ModelManager::cleanup()
{
    // Destroy our entities in the global registry
    auto& registry = ECSWorld::getRegistry();
    for (auto entity : m_instanceEntities)
    {
        if (registry.isValid(entity))
        {
            registry.destroy(entity);
        }
    }
    m_instanceEntities.clear();

    // Cleanup models
    for (auto &pair : m_models)
    {
        if (pair.second)
        {
            pair.second->cleanup();
        }
    }
    m_models.clear();

    m_pbrShader.cleanup();
    m_isInitialized = false;
}

bool ModelManager::setupPBRShader()
{
    std::cout << "[ModelManager] Loading PBR shaders..." << std::endl;

    std::vector<std::pair<std::string, std::string>> shaderPaths = {
        {"pbr_model.vert", "pbr_model.frag"},
        {"shaders/pbr_model.vert", "shaders/pbr_model.frag"},
        {"../shaders/pbr_model.vert", "../shaders/pbr_model.frag"},
        {"../../shaders/pbr_model.vert", "../../shaders/pbr_model.frag"},
        {"simple_test.vert", "simple_test.frag"}
    };

    for (const auto &paths : shaderPaths)
    {
        if (m_pbrShader.loadFromFiles(paths.first, paths.second))
        {
            std::cout << "[ModelManager] PBR shader loaded from: " << paths.first << std::endl;
            cacheUniformLocations();
            return true;
        }
    }

    std::cerr << "[ModelManager] Failed to load PBR shaders from any path" << std::endl;
    return false;
}

void ModelManager::cacheUniformLocations()
{
    GLuint shaderProgram = m_pbrShader.getProgram();
    if (shaderProgram == 0) return;

    m_uniforms.cameraPos = glGetUniformLocation(shaderProgram, "uCameraPos");
    m_uniforms.lightDir = glGetUniformLocation(shaderProgram, "uLightDir");
    m_uniforms.lightColor = glGetUniformLocation(shaderProgram, "uLightColor");
    m_uniforms.exposure = glGetUniformLocation(shaderProgram, "uExposure");
    m_uniforms.flashlightOn = glGetUniformLocation(shaderProgram, "uFlashlightOn");
    m_uniforms.flashlightPos = glGetUniformLocation(shaderProgram, "uFlashlightPos");
    m_uniforms.flashlightDir = glGetUniformLocation(shaderProgram, "uFlashlightDir");
    m_uniforms.flashlightCutoff = glGetUniformLocation(shaderProgram, "uFlashlightCutoff");
    m_uniforms.flashlightBrightness = glGetUniformLocation(shaderProgram, "uFlashlightBrightness");
    m_uniforms.flashlightColor = glGetUniformLocation(shaderProgram, "uFlashlightColor");
    m_uniforms.debugNormals = glGetUniformLocation(shaderProgram, "uDebugNormals");
    m_uniforms.fogEnabled = glGetUniformLocation(shaderProgram, "uFogEnabled");
    m_uniforms.fogColor = glGetUniformLocation(shaderProgram, "uFogColor");
    m_uniforms.fogDensity = glGetUniformLocation(shaderProgram, "uFogDensity");
    m_uniforms.fogDesaturationStrength = glGetUniformLocation(shaderProgram, "uFogDesaturationStrength");
    m_uniforms.fogAbsorptionDensity = glGetUniformLocation(shaderProgram, "uFogAbsorptionDensity");
    m_uniforms.fogAbsorptionStrength = glGetUniformLocation(shaderProgram, "uFogAbsorptionStrength");
    m_uniforms.backgroundColor = glGetUniformLocation(shaderProgram, "uBackgroundColor");

    std::cout << "[ModelManager] Cached " << 18 << " uniform locations" << std::endl;
}

bool ModelManager::loadModel(const std::string &filepath, const std::string &name)
{
    if (!m_isInitialized)
    {
        std::cerr << "[ModelManager] Not initialized" << std::endl;
        return false;
    }

    std::string modelName = name.empty() ? filepath : name;

    if (findModelIndex(modelName) != -1)
    {
        std::cout << "[ModelManager] Model '" << modelName << "' already loaded" << std::endl;
        return true;
    }

    std::cout << "[ModelManager] Loading GLTF model: " << filepath << " as '" << modelName << "'" << std::endl;

    auto model = std::make_unique<GLTFModel>();
    if (!model->loadFromFile(filepath))
    {
        std::cerr << "[ModelManager] Failed to load model: " << filepath << std::endl;
        return false;
    }

    m_models.emplace_back(modelName, std::move(model));

    std::cout << "[ModelManager] Model '" << modelName << "' loaded successfully" << std::endl;
    return true;
}

void ModelManager::removeModel(const std::string &name)
{
    int index = findModelIndex(name);
    if (index == -1)
    {
        std::cout << "[ModelManager] Model '" << name << "' not found" << std::endl;
        return;
    }

    // TODO: Remove all instances referencing this model
    // For now, just remove the model
    m_models[index].second->cleanup();
    m_models.erase(m_models.begin() + index);

    std::cout << "[ModelManager] Model '" << name << "' removed" << std::endl;
}

GLTFModel *ModelManager::getModel(const std::string &name)
{
    int index = findModelIndex(name);
    if (index == -1)
    {
        return nullptr;
    }
    return m_models[index].second.get();
}

int ModelManager::addModelInstance(const std::string &modelName, const glm::mat4 &transform)
{
    int modelIndex = findModelIndex(modelName);
    if (modelIndex == -1)
    {
        std::cerr << "[ModelManager] Model '" << modelName << "' not found" << std::endl;
        return -1;
    }

    GLTFModel *modelPtr = m_models[modelIndex].second.get();

    ecs::Entity entity = createModelInstanceEntity(modelPtr, transform);
    m_instanceEntities.push_back(entity);
    int instanceId = static_cast<int>(m_instanceEntities.size() - 1);

    std::cout << "[ModelManager] Added instance " << instanceId << " of model '" << modelName << "'" << std::endl;
    return instanceId;
}

void ModelManager::removeModelInstance(int instanceId)
{
    if (instanceId < 0 || instanceId >= static_cast<int>(m_instanceEntities.size()))
    {
        std::cerr << "[ModelManager] Invalid instance ID: " << instanceId << std::endl;
        return;
    }

    auto& registry = ECSWorld::getRegistry();
    ecs::Entity entity = m_instanceEntities[instanceId];
    if (registry.isValid(entity))
    {
        registry.destroy(entity);
    }
    m_instanceEntities.erase(m_instanceEntities.begin() + instanceId);

    std::cout << "[ModelManager] Removed instance " << instanceId << std::endl;
}

void ModelManager::setInstanceTransform(int instanceId, const glm::mat4 &transform)
{
    if (instanceId < 0 || instanceId >= static_cast<int>(m_instanceEntities.size()))
    {
        std::cerr << "[ModelManager] Invalid instance ID: " << instanceId << std::endl;
        return;
    }

    auto& registry = ECSWorld::getRegistry();
    ecs::Entity entity = m_instanceEntities[instanceId];
    if (auto* transformComp = registry.tryGet<ecs::TransformComponent>(entity))
    {
        transformComp->position = glm::vec3(transform[3]);
        transformComp->modelMatrix = transform;
        transformComp->dirty = false;
    }
}

void ModelManager::setInstanceVisibility(int instanceId, bool visible)
{
    if (instanceId < 0 || instanceId >= static_cast<int>(m_instanceEntities.size()))
    {
        std::cerr << "[ModelManager] Invalid instance ID: " << instanceId << std::endl;
        return;
    }

    auto& registry = ECSWorld::getRegistry();
    ecs::Entity entity = m_instanceEntities[instanceId];
    if (auto* renderable = registry.tryGet<ecs::RenderableComponent>(entity))
    {
        renderable->visible = visible;
    }
}

void ModelManager::render(const glm::mat4 &view, const glm::mat4 &projection,
                          const glm::vec3 &cameraPos, const glm::vec3 &lightDir,
                          const glm::vec3 &lightColor, const LightManager &lightManager)
{
    if (!m_isInitialized || m_instanceEntities.empty())
    {
        return;
    }

    GLuint shaderProgram = m_pbrShader.getProgram();
    if (shaderProgram == 0)
    {
        std::cerr << "[ModelManager] PBR shader not ready" << std::endl;
        return;
    }

    glUseProgram(shaderProgram);

    // Use cached uniform locations (no glGetUniformLocation calls per frame)
    if (m_uniforms.cameraPos >= 0)
        glUniform3fv(m_uniforms.cameraPos, 1, glm::value_ptr(cameraPos));
    if (m_uniforms.lightDir >= 0)
        glUniform3fv(m_uniforms.lightDir, 1, glm::value_ptr(lightDir));
    if (m_uniforms.lightColor >= 0)
        glUniform3fv(m_uniforms.lightColor, 1, glm::value_ptr(lightColor));
    if (m_uniforms.exposure >= 0)
        glUniform1f(m_uniforms.exposure, 1.0f);

    // Set flashlight uniforms from LightManager
    if (m_uniforms.flashlightOn >= 0)
        glUniform1i(m_uniforms.flashlightOn, lightManager.isFlashlightOn() ? 1 : 0);
    if (m_uniforms.flashlightPos >= 0)
        glUniform3fv(m_uniforms.flashlightPos, 1, glm::value_ptr(lightManager.getFlashlightPosition()));
    if (m_uniforms.flashlightDir >= 0)
        glUniform3fv(m_uniforms.flashlightDir, 1, glm::value_ptr(lightManager.getFlashlightDirection()));
    if (m_uniforms.flashlightCutoff >= 0)
        glUniform1f(m_uniforms.flashlightCutoff, lightManager.getFlashlightCutoff());
    if (m_uniforms.flashlightBrightness >= 0)
        glUniform1f(m_uniforms.flashlightBrightness, lightManager.getFlashlightBrightness());
    if (m_uniforms.flashlightColor >= 0)
        glUniform3fv(m_uniforms.flashlightColor, 1, glm::value_ptr(lightManager.getFlashlightColor()));

    // Set debug uniforms
    if (m_uniforms.debugNormals >= 0)
        glUniform1i(m_uniforms.debugNormals, 0);

    // Set fog uniforms
    if (m_uniforms.fogEnabled >= 0)
        glUniform1i(m_uniforms.fogEnabled, m_fogEnabled ? 1 : 0);
    if (m_uniforms.fogColor >= 0)
        glUniform3fv(m_uniforms.fogColor, 1, glm::value_ptr(m_fogColor));
    if (m_uniforms.fogDensity >= 0)
        glUniform1f(m_uniforms.fogDensity, m_fogDensity);
    if (m_uniforms.fogDesaturationStrength >= 0)
        glUniform1f(m_uniforms.fogDesaturationStrength, m_fogDesaturationStrength);
    if (m_uniforms.fogAbsorptionDensity >= 0)
        glUniform1f(m_uniforms.fogAbsorptionDensity, m_fogAbsorptionDensity);
    if (m_uniforms.fogAbsorptionStrength >= 0)
        glUniform1f(m_uniforms.fogAbsorptionStrength, m_fogAbsorptionStrength);
    if (m_uniforms.backgroundColor >= 0)
        glUniform3f(m_uniforms.backgroundColor, 0.08f, 0.1f, 0.12f);

    // Render each visible entity via ECS
    auto& registry = ECSWorld::getRegistry();
    int renderedCount = 0;
    int skippedNotVisible = 0;
    int skippedNoModel = 0;

    registry.each<ecs::TransformComponent, ecs::RenderableComponent, ModelRefComponent>(
        [&](ecs::Entity entity, ecs::TransformComponent& transformComp,
            ecs::RenderableComponent& renderable, ModelRefComponent& modelRef) {
            if (!modelRef.model)
            {
                skippedNoModel++;
                return;
            }
            if (!renderable.visible)
            {
                skippedNotVisible++;
                return;
            }

            modelRef.model->setTransform(transformComp.modelMatrix);
            modelRef.model->render(view, projection, cameraPos, lightDir, lightColor, shaderProgram);
            renderedCount++;
        });

    // Debug: print render stats once per second
    static float lastDebugTime = 0.0f;
    static int frameCount = 0;
    frameCount++;
    if (frameCount % 60 == 0) {
        std::cout << "[ModelManager] Render stats: rendered=" << renderedCount
                  << ", skippedNotVisible=" << skippedNotVisible
                  << ", skippedNoModel=" << skippedNoModel
                  << ", totalEntities=" << m_instanceEntities.size() << std::endl;
    }

    if (renderedCount == 0 && !m_instanceEntities.empty())
    {
        static int warnCount = 0;
        if (++warnCount <= 3) {
            std::cout << "[ModelManager] WARNING: No instances rendered despite having "
                      << m_instanceEntities.size() << " instances" << std::endl;
        }
    }

    glUseProgram(0);
}

size_t ModelManager::getTotalVertexCount() const
{
    size_t total = 0;
    auto& registry = ECSWorld::getRegistry();

    registry.each<ecs::RenderableComponent, ModelRefComponent>(
        [&](ecs::RenderableComponent& renderable, ModelRefComponent& modelRef) {
            if (modelRef.model)
            {
                total += modelRef.model->getVertexCount();
            }
        });

    return total;
}

size_t ModelManager::getTotalTriangleCount() const
{
    size_t total = 0;
    auto& registry = ECSWorld::getRegistry();

    registry.each<ecs::RenderableComponent, ModelRefComponent>(
        [&](ecs::RenderableComponent& renderable, ModelRefComponent& modelRef) {
            if (modelRef.model)
            {
                total += modelRef.model->getTriangleCount();
            }
        });

    return total;
}

void ModelManager::printStats() const
{
    std::cout << "\n=== MODEL MANAGER STATS ===" << std::endl;
    std::cout << "Loaded Models: " << m_models.size() << std::endl;
    std::cout << "Active Instances: " << m_instanceEntities.size() << std::endl;
    std::cout << "Total Vertices: " << getTotalVertexCount() << std::endl;
    std::cout << "Total Triangles: " << getTotalTriangleCount() << std::endl;

    for (size_t i = 0; i < m_models.size(); ++i)
    {
        std::cout << "  Model " << i << " (" << m_models[i].first << "): "
                  << m_models[i].second->getVertexCount() << " vertices, "
                  << m_models[i].second->getTriangleCount() << " triangles" << std::endl;
    }
    std::cout << "===========================" << std::endl;
}

int ModelManager::findModelIndex(const std::string &name)
{
    for (size_t i = 0; i < m_models.size(); ++i)
    {
        if (m_models[i].first == name)
        {
            return static_cast<int>(i);
        }
    }
    return -1;
}

// =========== ECS Implementation ===========

ecs::Entity ModelManager::createModelInstanceEntity(GLTFModel* model, const glm::mat4& transform)
{
    auto& registry = ECSWorld::getRegistry();
    ecs::Entity entity = registry.create();

    // Add TransformComponent with the model matrix
    auto& transformComp = registry.add<ecs::TransformComponent>(entity);
    transformComp.position = glm::vec3(transform[3]);
    transformComp.modelMatrix = transform;
    transformComp.dirty = false;

    // Add RenderableComponent
    auto& renderable = registry.add<ecs::RenderableComponent>(entity);
    renderable.type = ecs::RenderableType::GLTF_MODEL;
    renderable.visible = true;
    renderable.castShadow = true;
    renderable.receiveShadow = true;

    // Add custom ModelRefComponent for linking to GLTFModel
    auto& modelRef = registry.add<ModelRefComponent>(entity);
    modelRef.model = model;

    return entity;
}
