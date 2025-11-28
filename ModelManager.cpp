#include "ModelManager.h"
#include <iostream>

ModelManager::ModelManager()
    : m_isInitialized(false), m_fogEnabled(true), m_fogColor(0.0f, 0.0f, 0.0f), m_fogDensity(0.01f), m_fogDesaturationStrength(1.0f), m_fogAbsorptionDensity(0.02f), m_fogAbsorptionStrength(0.8f)
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

    std::cout << "Initializing ModelManager..." << std::endl;

    if (!setupPBRShader())
    {
        std::cerr << "Failed to setup PBR shader" << std::endl;
        return false;
    }

    m_isInitialized = true;
    std::cout << "ModelManager initialized successfully!" << std::endl;
    return true;
}

void ModelManager::cleanup()
{
    // Don't cleanup instances since they reference models owned by m_models
    m_instances.clear();

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
    std::cout << "Loading PBR shaders..." << std::endl;

    // Try different paths for shaders
    std::vector<std::pair<std::string, std::string>> shaderPaths = {
        {"pbr_model.vert", "pbr_model.frag"},                             // Direct in output directory
        {"shaders/pbr_model.vert", "shaders/pbr_model.frag"},             // In shaders subdirectory
        {"../shaders/pbr_model.vert", "../shaders/pbr_model.frag"},       // One level up
        {"../../shaders/pbr_model.vert", "../../shaders/pbr_model.frag"}, // Two levels up
        {"simple_test.vert", "simple_test.frag"}                          // Fallback to simple test shaders
    };

    for (const auto &paths : shaderPaths)
    {
        std::cout << "Trying to load shaders: " << paths.first << ", " << paths.second << std::endl;
        if (m_pbrShader.loadFromFiles(paths.first, paths.second))
        {
            std::cout << "PBR shader loaded successfully from: " << paths.first << std::endl;
            return true;
        }
    }

    std::cerr << "Failed to load PBR shaders from any path" << std::endl;
    return false;
}

bool ModelManager::loadModel(const std::string &filepath, const std::string &name)
{
    if (!m_isInitialized)
    {
        std::cerr << "ModelManager not initialized" << std::endl;
        return false;
    }

    std::string modelName = name.empty() ? filepath : name;

    // Check if model already exists
    if (findModelIndex(modelName) != -1)
    {
        std::cout << "Model '" << modelName << "' already loaded" << std::endl;
        return true;
    }

    std::cout << "Loading GLTF model: " << filepath << " as '" << modelName << "'" << std::endl;

    auto model = std::make_unique<GLTFModel>();
    if (!model->loadFromFile(filepath))
    {
        std::cerr << "Failed to load model: " << filepath << std::endl;
        return false;
    }

    m_models.emplace_back(modelName, std::move(model));

    std::cout << "Model '" << modelName << "' loaded successfully" << std::endl;
    return true;
}

void ModelManager::removeModel(const std::string &name)
{
    int index = findModelIndex(name);
    if (index == -1)
    {
        std::cout << "Model '" << name << "' not found" << std::endl;
        return;
    }

    // Remove all instances of this model
    m_instances.erase(
        std::remove_if(m_instances.begin(), m_instances.end(),
                       [&](const ModelInstance &instance)
                       {
                           // We can't directly compare model pointers, so we'll need to find a better way
                           // For now, we'll remove all instances when removing a model
                           return true;
                       }),
        m_instances.end());

    // Remove the model
    m_models[index].second->cleanup();
    m_models.erase(m_models.begin() + index);

    std::cout << "Model '" << name << "' removed" << std::endl;
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
        std::cerr << "Model '" << modelName << "' not found" << std::endl;
        return -1;
    }

    // Create an instance that references the loaded model
    GLTFModel *modelPtr = m_models[modelIndex].second.get();
    m_instances.emplace_back(modelPtr, transform);

    int instanceId = static_cast<int>(m_instances.size() - 1);
    std::cout << "Added instance " << instanceId << " of model '" << modelName << "'" << std::endl;

    return instanceId;
}

void ModelManager::removeModelInstance(int instanceId)
{
    if (instanceId < 0 || instanceId >= static_cast<int>(m_instances.size()))
    {
        std::cerr << "Invalid instance ID: " << instanceId << std::endl;
        return;
    }

    // Don't cleanup the model since it's owned by m_models
    m_instances.erase(m_instances.begin() + instanceId);

    std::cout << "Removed instance " << instanceId << std::endl;
}

void ModelManager::setInstanceTransform(int instanceId, const glm::mat4 &transform)
{
    if (instanceId < 0 || instanceId >= static_cast<int>(m_instances.size()))
    {
        std::cerr << "Invalid instance ID: " << instanceId << std::endl;
        return;
    }

    m_instances[instanceId].transform = transform;
}

void ModelManager::setInstanceVisibility(int instanceId, bool visible)
{
    if (instanceId < 0 || instanceId >= static_cast<int>(m_instances.size()))
    {
        std::cerr << "Invalid instance ID: " << instanceId << std::endl;
        return;
    }

    m_instances[instanceId].isVisible = visible;
}

void ModelManager::render(const glm::mat4 &view, const glm::mat4 &projection,
                          const glm::vec3 &cameraPos, const glm::vec3 &lightDir,
                          const glm::vec3 &lightColor, const LightManager &lightManager)
{
    if (!m_isInitialized || m_instances.empty())
    {
        return;
    }

    GLuint shaderProgram = m_pbrShader.getProgram();
    if (shaderProgram == 0)
    {
        std::cerr << "PBR shader not ready" << std::endl;
        return;
    }

    // Set common uniforms
    glUseProgram(shaderProgram);

    GLint locCameraPos = glGetUniformLocation(shaderProgram, "uCameraPos");
    GLint locLightDir = glGetUniformLocation(shaderProgram, "uLightDir");
    GLint locLightColor = glGetUniformLocation(shaderProgram, "uLightColor");
    GLint locExposure = glGetUniformLocation(shaderProgram, "uExposure");

    // Flashlight uniforms
    GLint locFlashlightOn = glGetUniformLocation(shaderProgram, "uFlashlightOn");
    GLint locFlashlightPos = glGetUniformLocation(shaderProgram, "uFlashlightPos");
    GLint locFlashlightDir = glGetUniformLocation(shaderProgram, "uFlashlightDir");
    GLint locFlashlightCutoff = glGetUniformLocation(shaderProgram, "uFlashlightCutoff");
    GLint locFlashlightBrightness = glGetUniformLocation(shaderProgram, "uFlashlightBrightness");
    GLint locFlashlightColor = glGetUniformLocation(shaderProgram, "uFlashlightColor");

    // Debug uniforms
    GLint locDebugNormals = glGetUniformLocation(shaderProgram, "uDebugNormals");

    // Fog uniforms
    GLint locFogEnabled = glGetUniformLocation(shaderProgram, "uFogEnabled");
    GLint locFogColor = glGetUniformLocation(shaderProgram, "uFogColor");
    GLint locFogDensity = glGetUniformLocation(shaderProgram, "uFogDensity");
    GLint locFogDesaturationStrength = glGetUniformLocation(shaderProgram, "uFogDesaturationStrength");
    GLint locFogAbsorptionDensity = glGetUniformLocation(shaderProgram, "uFogAbsorptionDensity");
    GLint locFogAbsorptionStrength = glGetUniformLocation(shaderProgram, "uFogAbsorptionStrength");
    GLint locBackgroundColor = glGetUniformLocation(shaderProgram, "uBackgroundColor");

    if (locCameraPos >= 0)
        glUniform3fv(locCameraPos, 1, glm::value_ptr(cameraPos));
    if (locLightDir >= 0)
        glUniform3fv(locLightDir, 1, glm::value_ptr(lightDir));
    if (locLightColor >= 0)
        glUniform3fv(locLightColor, 1, glm::value_ptr(lightColor));
    if (locExposure >= 0)
        glUniform1f(locExposure, 1.0f);

    // Debug flashlight state (gated by runtime env var OPENGL_ADV_DEBUG_LOGS)
    static bool gDebugLogs = (std::getenv("OPENGL_ADV_DEBUG_LOGS") != nullptr);
    static int debugCounter = 0;
    if (gDebugLogs)
    {
        if (debugCounter % 60 == 0)
        {
            std::cout << "=== SHADER FLASHLIGHT DEBUG ===" << std::endl;
            std::cout << "Shader Program: " << shaderProgram << std::endl;
            std::cout << "Flashlight On: " << (lightManager.isFlashlightOn() ? "YES" : "NO") << std::endl;
            std::cout << "Uniform Locations - On:" << locFlashlightOn << " Pos:" << locFlashlightPos
                      << " Dir:" << locFlashlightDir << " Cutoff:" << locFlashlightCutoff
                      << " Brightness:" << locFlashlightBrightness << " Color:" << locFlashlightColor << std::endl;
            std::cout << "Flashlight Brightness: " << lightManager.getFlashlightBrightness() << std::endl;
            std::cout << "Flashlight Cutoff: " << lightManager.getFlashlightCutoff() << std::endl;
        }
        debugCounter++;
    }

    // Set flashlight uniforms from LightManager
    if (locFlashlightOn >= 0)
        glUniform1i(locFlashlightOn, lightManager.isFlashlightOn() ? 1 : 0);
    if (locFlashlightPos >= 0)
        glUniform3fv(locFlashlightPos, 1, glm::value_ptr(lightManager.getFlashlightPosition()));
    if (locFlashlightDir >= 0)
        glUniform3fv(locFlashlightDir, 1, glm::value_ptr(lightManager.getFlashlightDirection()));
    if (locFlashlightCutoff >= 0)
        glUniform1f(locFlashlightCutoff, lightManager.getFlashlightCutoff());
    if (locFlashlightBrightness >= 0)
        glUniform1f(locFlashlightBrightness, lightManager.getFlashlightBrightness());
    if (locFlashlightColor >= 0)
        glUniform3fv(locFlashlightColor, 1, glm::value_ptr(lightManager.getFlashlightColor()));

    // Set debug uniforms
    if (locDebugNormals >= 0)
        glUniform1i(locDebugNormals, 0); // Disabled by default

    // Set fog uniforms
    if (locFogEnabled >= 0)
        glUniform1i(locFogEnabled, m_fogEnabled ? 1 : 0);
    if (locFogColor >= 0)
        glUniform3fv(locFogColor, 1, glm::value_ptr(m_fogColor));
    if (locFogDensity >= 0)
        glUniform1f(locFogDensity, m_fogDensity);
    if (locFogDesaturationStrength >= 0)
        glUniform1f(locFogDesaturationStrength, m_fogDesaturationStrength);
    if (locFogAbsorptionDensity >= 0)
        glUniform1f(locFogAbsorptionDensity, m_fogAbsorptionDensity);
    if (locFogAbsorptionStrength >= 0)
        glUniform1f(locFogAbsorptionStrength, m_fogAbsorptionStrength);
    if (locBackgroundColor >= 0)
        glUniform3f(locBackgroundColor, 0.08f, 0.1f, 0.12f); // Match renderer clear color

    // Render each visible instance
    int renderedCount = 0;

    for (size_t i = 0; i < m_instances.size(); ++i)
    {
        const auto &instance = m_instances[i];

        if (!instance.isVisible || !instance.model)
        {
            continue;
        }

        // Set instance transform
        instance.model->setTransform(instance.transform);

        // Render the model
        instance.model->render(view, projection, cameraPos, lightDir, lightColor, shaderProgram);
        renderedCount++;
    }

    // Only log if there's an issue
    if (renderedCount == 0 && !m_instances.empty())
    {
        std::cout << "WARNING: No model instances rendered despite having " << m_instances.size() << " instances" << std::endl;
    }

    glUseProgram(0);
}

size_t ModelManager::getTotalVertexCount() const
{
    size_t total = 0;
    for (const auto &instance : m_instances)
    {
        if (instance.model)
        {
            total += instance.model->getVertexCount();
        }
    }
    return total;
}

size_t ModelManager::getTotalTriangleCount() const
{
    size_t total = 0;
    for (const auto &instance : m_instances)
    {
        if (instance.model)
        {
            total += instance.model->getTriangleCount();
        }
    }
    return total;
}

void ModelManager::printStats() const
{
    std::cout << "\n=== MODEL MANAGER STATS ===" << std::endl;
    std::cout << "Loaded Models: " << m_models.size() << std::endl;
    std::cout << "Active Instances: " << m_instances.size() << std::endl;
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
