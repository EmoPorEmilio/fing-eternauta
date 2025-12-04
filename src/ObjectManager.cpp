#include "ObjectManager.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>

ObjectManager::ObjectManager() : isInitialized(false),
                                 highLodVao(0), highLodVbo(0), highLodEbo(0),
                                 mediumLodVao(0), mediumLodVbo(0), mediumLodEbo(0),
                                 lowLodVao(0), lowLodVbo(0), lowLodEbo(0),
                                 highLodInstanceVbo(0), mediumLodInstanceVbo(0), lowLodInstanceVbo(0),
                                 shaderProgram(0), frameTime(0.0f), fps(0.0f),
                                 frameCount(0), lastStatsTime(0.0f),
                                 cullingEnabled(true), lodEnabled(false),
                                 fogEnabled(true), fogColor(0.0f, 0.0f, 0.0f), fogDensity(0.01f),
                                 fogDesaturationStrength(1.0f), fogAbsorptionDensity(0.02f), fogAbsorptionStrength(0.8f)
{
}

ObjectManager::~ObjectManager()
{
    cleanup();
}

void ObjectManager::initialize(int objectCount)
{
    if (isInitialized)
    {
        cleanup();
    }

    std::cout << "[ObjectManager] Initializing with " << objectCount << " objects (ECS mode)" << std::endl;

    // Setup OpenGL resources first
    setupLODGeometry();
    setupShader();

    // Initialize ECS systems if not already done
    initializeECSSystems();

    // Create prism entities
    createPrismEntities(objectCount);

    std::cout << "[ObjectManager] Initialization complete. Created " << m_entities.size() << " entities." << std::endl;

    // Subscribe to configuration events
    subscribeToEvents();

    isInitialized = true;
}

void ObjectManager::initializeECSSystems()
{
    auto& systems = ECSWorld::getSystems();

    // Check if systems already exist
    m_lodSystem = systems.getSystem<ecs::LODSystem>();
    m_cullingSystem = systems.getSystem<ecs::CullingSystem>();

    // Add systems if they don't exist yet
    if (!m_lodSystem)
    {
        m_lodSystem = &systems.addSystem<ecs::LODSystem>();
        std::cout << "[ObjectManager] Added LODSystem to ECSWorld" << std::endl;
    }

    if (!m_cullingSystem)
    {
        m_cullingSystem = &systems.addSystem<ecs::CullingSystem>();
        std::cout << "[ObjectManager] Added CullingSystem to ECSWorld" << std::endl;
    }

    // Configure systems
    m_lodSystem->setLODEnabled(lodEnabled);
    m_cullingSystem->setCullingEnabled(cullingEnabled);

    // Initialize systems
    systems.init(ECSWorld::getRegistry());
}

void ObjectManager::update(const glm::vec3 &cameraPos, float cullDistance, float highLodDistance, float mediumLodDistance, float deltaTime)
{
    if (!isInitialized)
        return;

    // Update performance stats
    if (deltaTime > 0.0f)
        updatePerformanceStats(deltaTime);

    // Configure systems with current parameters
    if (m_lodSystem)
    {
        m_lodSystem->setCameraPosition(cameraPos);
        m_lodSystem->setLODEnabled(lodEnabled);
    }
    if (m_cullingSystem)
    {
        m_cullingSystem->setCameraPosition(cameraPos);
        m_cullingSystem->setCullDistance(cullDistance);
        m_cullingSystem->setCullingEnabled(cullingEnabled);
    }

    // Systems are updated via ECSWorld::update() called from Application
    // We don't need to call m_systems.update() here anymore
}

void ObjectManager::render(const glm::mat4 &view, const glm::mat4 &projection, const glm::vec3 &cameraPos, const glm::vec3 &cameraFront, const LightManager &lightManager, GLuint textureID)
{
    if (!isInitialized)
        return;

    glUseProgram(shaderProgram);

    // Use cached uniform locations (no glGetUniformLocation calls per frame)
    glUniformMatrix4fv(m_uniforms.view, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(m_uniforms.projection, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(m_uniforms.viewPos, cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform3f(m_uniforms.objectColor, 1.0f, 0.0f, 0.0f); // Red color
    glUniform1i(m_uniforms.useTexture, 0);                 // No texture for objects

    // Set flashlight uniforms
    glUniform1i(m_uniforms.flashlightOn, lightManager.isFlashlightOn() ? 1 : 0);
    glUniform3f(m_uniforms.flashlightPos, cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform3f(m_uniforms.flashlightDir, cameraFront.x, cameraFront.y, cameraFront.z);
    glUniform1f(m_uniforms.flashlightCutoff, lightManager.getFlashlightCutoff());
    glUniform1f(m_uniforms.flashlightBrightness, lightManager.getFlashlightBrightness());
    glm::vec3 flColor = lightManager.getFlashlightColor();
    glUniform3f(m_uniforms.flashlightColor, flColor.x, flColor.y, flColor.z);

    // Set fog uniforms
    glUniform1i(m_uniforms.fogEnabled, fogEnabled ? 1 : 0);
    glUniform3f(m_uniforms.fogColor, fogColor.x, fogColor.y, fogColor.z);
    glUniform1f(m_uniforms.fogDensity, fogDensity);
    glUniform1f(m_uniforms.fogDesaturationStrength, fogDesaturationStrength);
    glUniform1f(m_uniforms.fogAbsorptionDensity, fogAbsorptionDensity);
    glUniform1f(m_uniforms.fogAbsorptionStrength, fogAbsorptionStrength);
    glUniform3f(m_uniforms.backgroundColor, 0.08f, 0.1f, 0.12f);

    // Gather instances per LOD - reserve smart estimates
    std::vector<glm::mat4> highInstances;
    std::vector<glm::mat4> mediumInstances;
    std::vector<glm::mat4> lowInstances;

    size_t totalObjects = m_entities.size();
    highInstances.reserve(totalObjects / 10);
    mediumInstances.reserve(totalObjects / 4);
    lowInstances.reserve(totalObjects * 2 / 3);

    if (!lodEnabled)
    {
        // LOD disabled: all visible entities use HIGH LOD
        auto& registry = ECSWorld::getRegistry();
        registry.each<ecs::TransformComponent, ecs::RenderableComponent>(
            [&](ecs::TransformComponent& transform, ecs::RenderableComponent& renderable) {
                if (renderable.visible)
                    highInstances.push_back(transform.modelMatrix);
            });
    }
    else
    {
        // LOD enabled: gather by LOD level
        gatherInstanceMatrices(highInstances, mediumInstances, lowInstances);
    }

    // Render each LOD level with instancing
    if (!highInstances.empty())
        renderLODLevelInstanced(LODLevel::HIGH, highInstances);
    if (!mediumInstances.empty())
        renderLODLevelInstanced(LODLevel::MEDIUM, mediumInstances);
    if (!lowInstances.empty())
        renderLODLevelInstanced(LODLevel::LOW, lowInstances);

    // Debug info: show LOD distribution occasionally
    static int lodDebugCounter = 0;
    if (++lodDebugCounter >= 300)
    {
        lodDebugCounter = 0;
        std::cout << "[ObjectManager] LOD Distribution - High: " << highInstances.size()
                  << ", Medium: " << mediumInstances.size()
                  << ", Low: " << lowInstances.size()
                  << " (Total visible: " << (highInstances.size() + mediumInstances.size() + lowInstances.size()) << ")" << std::endl;
    }
}

void ObjectManager::setObjectCount(int count)
{
    if (count != static_cast<int>(m_entities.size()))
    {
        initialize(count);
    }
}

int ObjectManager::getObjectCount() const
{
    return static_cast<int>(m_entities.size());
}

void ObjectManager::setupShader()
{
    shaderProgram = AssetManager::loadShaderProgram(
        "object_instanced.vert",
        "object_instanced.frag",
        "ObjectManager_instanced");

    if (shaderProgram == 0)
    {
        std::cerr << "[ObjectManager] CRITICAL ERROR: Failed to load shader program!" << std::endl;
    }
    else
    {
        std::cout << "[ObjectManager] Shader program loaded (ID: " << shaderProgram << ")" << std::endl;
        cacheUniformLocations();
    }

    AssetManager::checkGLError("ObjectManager::setupShader");
}

void ObjectManager::cacheUniformLocations()
{
    if (shaderProgram == 0) return;

    m_uniforms.view = glGetUniformLocation(shaderProgram, "uView");
    m_uniforms.projection = glGetUniformLocation(shaderProgram, "uProj");
    m_uniforms.viewPos = glGetUniformLocation(shaderProgram, "uViewPos");
    m_uniforms.objectColor = glGetUniformLocation(shaderProgram, "uObjectColor");
    m_uniforms.useTexture = glGetUniformLocation(shaderProgram, "uUseTexture");
    m_uniforms.flashlightOn = glGetUniformLocation(shaderProgram, "uFlashlightOn");
    m_uniforms.flashlightPos = glGetUniformLocation(shaderProgram, "uFlashlightPos");
    m_uniforms.flashlightDir = glGetUniformLocation(shaderProgram, "uFlashlightDir");
    m_uniforms.flashlightCutoff = glGetUniformLocation(shaderProgram, "uFlashlightCutoff");
    m_uniforms.flashlightBrightness = glGetUniformLocation(shaderProgram, "uFlashlightBrightness");
    m_uniforms.flashlightColor = glGetUniformLocation(shaderProgram, "uFlashlightColor");
    m_uniforms.fogEnabled = glGetUniformLocation(shaderProgram, "uFogEnabled");
    m_uniforms.fogColor = glGetUniformLocation(shaderProgram, "uFogColor");
    m_uniforms.fogDensity = glGetUniformLocation(shaderProgram, "uFogDensity");
    m_uniforms.fogDesaturationStrength = glGetUniformLocation(shaderProgram, "uFogDesaturationStrength");
    m_uniforms.fogAbsorptionDensity = glGetUniformLocation(shaderProgram, "uFogAbsorptionDensity");
    m_uniforms.fogAbsorptionStrength = glGetUniformLocation(shaderProgram, "uFogAbsorptionStrength");
    m_uniforms.backgroundColor = glGetUniformLocation(shaderProgram, "uBackgroundColor");

    std::cout << "[ObjectManager] Cached " << 18 << " uniform locations" << std::endl;
}

void ObjectManager::setupLODGeometry()
{
    highLodPrism.generateRandomizedHighLODGeometry(12345);
    mediumLodPrism.generateGeometry(LODLevel::MEDIUM);
    lowLodPrism.generateGeometry(LODLevel::LOW);

    setupLODVAO(highLodVao, highLodVbo, highLodEbo, highLodInstanceVbo, highLodPrism);
    setupLODVAO(mediumLodVao, mediumLodVbo, mediumLodEbo, mediumLodInstanceVbo, mediumLodPrism);
    setupLODVAO(lowLodVao, lowLodVbo, lowLodEbo, lowLodInstanceVbo, lowLodPrism);

    std::cout << "[ObjectManager] LOD Geometry: High=" << highLodPrism.getTriangleCount()
              << " tris, Medium=" << mediumLodPrism.getTriangleCount()
              << " tris, Low=" << lowLodPrism.getTriangleCount() << " tris" << std::endl;
}

void ObjectManager::setupLODVAO(GLuint &vao, GLuint &vbo, GLuint &ebo, GLuint &instanceVbo, const Prism &prism)
{
    const auto &vertices = prism.getVertices();
    const auto &indices = prism.getIndices();

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Prism::Vertex)), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Prism::Vertex), (void *)0);

    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Prism::Vertex), (void *)offsetof(Prism::Vertex, normal));

    // UV attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Prism::Vertex), (void *)offsetof(Prism::Vertex, uv));

    // Instance buffer for model matrices (mat4 -> 4 vec4 attributes)
    glGenBuffers(1, &instanceVbo);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVbo);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    std::size_t vec4Size = sizeof(glm::vec4);
    GLsizei stride = sizeof(glm::mat4);

    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glVertexAttribDivisor(3, 1);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void *)(1 * vec4Size));
    glVertexAttribDivisor(4, 1);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, stride, (void *)(2 * vec4Size));
    glVertexAttribDivisor(5, 1);
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride, (void *)(3 * vec4Size));
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);
}

void ObjectManager::renderLODLevelInstanced(LODLevel lod, const std::vector<glm::mat4> &instanceMatrices)
{
    GLuint vao = 0;
    GLuint *instanceVboPtr = nullptr;
    const Prism *prism = nullptr;

    switch (lod)
    {
    case LODLevel::HIGH:
        vao = highLodVao;
        prism = &highLodPrism;
        instanceVboPtr = &highLodInstanceVbo;
        break;
    case LODLevel::MEDIUM:
        vao = mediumLodVao;
        prism = &mediumLodPrism;
        instanceVboPtr = &mediumLodInstanceVbo;
        break;
    case LODLevel::LOW:
        vao = lowLodVao;
        prism = &lowLodPrism;
        instanceVboPtr = &lowLodInstanceVbo;
        break;
    }

    if (!prism || vao == 0 || instanceVboPtr == nullptr)
        return;

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, *instanceVboPtr);
    glBufferData(GL_ARRAY_BUFFER, instanceMatrices.size() * sizeof(glm::mat4), instanceMatrices.data(), GL_DYNAMIC_DRAW);

    glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(prism->getIndices().size()), GL_UNSIGNED_INT, 0, static_cast<GLsizei>(instanceMatrices.size()));

    glBindVertexArray(0);
}

void ObjectManager::cleanup()
{
    // Unsubscribe from events
    unsubscribeFromEvents();

    // Destroy our entities in the global registry
    auto& registry = ECSWorld::getRegistry();
    for (auto entity : m_entities)
    {
        if (registry.isValid(entity))
        {
            registry.destroy(entity);
        }
    }
    m_entities.clear();

    m_lodSystem = nullptr;
    m_cullingSystem = nullptr;

    // Clean up OpenGL resources
    if (highLodVao != 0)
    {
        glDeleteVertexArrays(1, &highLodVao);
        glDeleteBuffers(1, &highLodVbo);
        glDeleteBuffers(1, &highLodEbo);
        glDeleteBuffers(1, &highLodInstanceVbo);
        highLodVao = highLodVbo = highLodEbo = highLodInstanceVbo = 0;
    }
    if (mediumLodVao != 0)
    {
        glDeleteVertexArrays(1, &mediumLodVao);
        glDeleteBuffers(1, &mediumLodVbo);
        glDeleteBuffers(1, &mediumLodEbo);
        glDeleteBuffers(1, &mediumLodInstanceVbo);
        mediumLodVao = mediumLodVbo = mediumLodEbo = mediumLodInstanceVbo = 0;
    }
    if (lowLodVao != 0)
    {
        glDeleteVertexArrays(1, &lowLodVao);
        glDeleteBuffers(1, &lowLodVbo);
        glDeleteBuffers(1, &lowLodEbo);
        glDeleteBuffers(1, &lowLodInstanceVbo);
        lowLodVao = lowLodVbo = lowLodEbo = lowLodInstanceVbo = 0;
    }
    if (shaderProgram != 0)
    {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }

    isInitialized = false;
}

void ObjectManager::updatePerformanceStats(float deltaTime)
{
    frameTime = deltaTime;
    fps = 1.0f / deltaTime;
    frameCount++;

    static float totalTime = 0.0f;
    totalTime += deltaTime;

    if (totalTime >= STATS_INTERVAL)
    {
        printPerformanceStats();
        totalTime = 0.0f;
    }
}

void ObjectManager::printPerformanceStats()
{
    auto& registry = ECSWorld::getRegistry();

    int highLodCount = 0, mediumLodCount = 0, lowLodCount = 0;

    registry.each<ecs::RenderableComponent, ecs::LODComponent>([&](ecs::RenderableComponent& renderable, ecs::LODComponent& lod) {
        if (!renderable.visible) return;
        switch (lod.currentLevel) {
            case LODLevel::HIGH: highLodCount++; break;
            case LODLevel::MEDIUM: mediumLodCount++; break;
            case LODLevel::LOW: lowLodCount++; break;
        }
    });

    int totalVisible = highLodCount + mediumLodCount + lowLodCount;
    if (totalVisible == 0) totalVisible = 1;

    size_t highVertices = highLodCount * highLodPrism.getVertexCount();
    size_t mediumVertices = mediumLodCount * mediumLodPrism.getVertexCount();
    size_t lowVertices = lowLodCount * lowLodPrism.getVertexCount();
    size_t totalVertices = highVertices + mediumVertices + lowVertices;

    size_t highTriangles = highLodCount * highLodPrism.getTriangleCount();
    size_t mediumTriangles = mediumLodCount * mediumLodPrism.getTriangleCount();
    size_t lowTriangles = lowLodCount * lowLodPrism.getTriangleCount();
    size_t totalTriangles = highTriangles + mediumTriangles + lowTriangles;

    std::cout << "\n=== PERFORMANCE STATS ===" << std::endl;
    std::cout << "FPS: " << std::fixed << std::setprecision(1) << fps << std::endl;
    std::cout << "Frame Time: " << std::fixed << std::setprecision(3) << (frameTime * 1000.0f) << "ms" << std::endl;
    std::cout << "Total Objects: " << m_entities.size() << std::endl;
    std::cout << "Visible Objects: " << totalVisible << std::endl;
    std::cout << "LOD Distribution:" << std::endl;
    std::cout << "  High: " << highLodCount << " (" << std::fixed << std::setprecision(1) << (highLodCount * 100.0f / totalVisible) << "%)" << std::endl;
    std::cout << "  Medium: " << mediumLodCount << " (" << std::fixed << std::setprecision(1) << (mediumLodCount * 100.0f / totalVisible) << "%)" << std::endl;
    std::cout << "  Low: " << lowLodCount << " (" << std::fixed << std::setprecision(1) << (lowLodCount * 100.0f / totalVisible) << "%)" << std::endl;
    std::cout << "Geometry: " << totalVertices << " vertices, " << totalTriangles << " triangles" << std::endl;
    std::cout << "Culled: " << (m_entities.size() - totalVisible) << std::endl;
    std::cout << "========================" << std::endl;
}

// ============================================================================
// ECS Implementation
// ============================================================================

ecs::Entity ObjectManager::createPrismEntity(const glm::vec3& position)
{
    auto& registry = ECSWorld::getRegistry();
    ecs::Entity entity = registry.create();

    // Add Transform component
    auto& transform = registry.add<ecs::TransformComponent>(entity, position);
    transform.updateModelMatrix();

    // Add Renderable component
    auto& renderable = registry.add<ecs::RenderableComponent>(entity, ecs::RenderableType::INSTANCED_PRISM);
    renderable.shaderProgram = shaderProgram;

    // Add LOD component with distance thresholds
    registry.add<ecs::LODComponent>(entity, HIGH_LOD_DISTANCE, MEDIUM_LOD_DISTANCE);

    // Add BatchGroup component for instancing
    registry.add<ecs::BatchGroupComponent>(entity, ecs::BatchID::PRISM);

    return entity;
}

void ObjectManager::createPrismEntities(int count)
{
    m_entities.clear();
    m_entities.reserve(count);

    float gridSpacing = MIN_DISTANCE;
    int gridSize = static_cast<int>(std::ceil(std::sqrt(count)));

    float startX = -(gridSize * gridSpacing) / 2.0f;
    float startZ = -(gridSize * gridSpacing) / 2.0f;

    int entitiesCreated = 0;
    const int progressInterval = std::max(1, count / 20);

    std::cout << "[ObjectManager] Creating " << count << " prism entities..." << std::endl;

    for (int row = 0; row < gridSize && entitiesCreated < count; row++)
    {
        for (int col = 0; col < gridSize && entitiesCreated < count; col++)
        {
            glm::vec3 position(
                startX + col * gridSpacing,
                0.5f,
                startZ + row * gridSpacing);

            ecs::Entity entity = createPrismEntity(position);
            m_entities.push_back(entity);
            entitiesCreated++;

            if (entitiesCreated % progressInterval == 0 || entitiesCreated == count)
            {
                float progress = (float)entitiesCreated / count * 100.0f;
                int barWidth = 50;
                int filled = static_cast<int>((progress / 100.0f) * barWidth);

                std::cout << "\r[";
                for (int i = 0; i < barWidth; i++)
                {
                    if (i < filled) std::cout << "=";
                    else if (i == filled) std::cout << ">";
                    else std::cout << " ";
                }
                std::cout << "] " << std::fixed << std::setprecision(1) << progress << "%" << std::flush;
            }
        }
    }

    std::cout << std::endl;
}

void ObjectManager::gatherInstanceMatrices(std::vector<glm::mat4>& highInstances,
                                           std::vector<glm::mat4>& mediumInstances,
                                           std::vector<glm::mat4>& lowInstances)
{
    auto& registry = ECSWorld::getRegistry();

    registry.each<ecs::TransformComponent, ecs::RenderableComponent, ecs::LODComponent>(
        [&](ecs::TransformComponent& transform, ecs::RenderableComponent& renderable, ecs::LODComponent& lod) {
            if (!renderable.visible) return;

            switch (lod.currentLevel) {
                case LODLevel::HIGH:
                    highInstances.push_back(transform.modelMatrix);
                    break;
                case LODLevel::MEDIUM:
                    mediumInstances.push_back(transform.modelMatrix);
                    break;
                case LODLevel::LOW:
                    lowInstances.push_back(transform.modelMatrix);
                    break;
            }
        });
}

// =========== Event Handling ===========

void ObjectManager::subscribeToEvents()
{
    auto& bus = events::EventBus::instance();

    // Subscribe to fog configuration changes
    m_fogSubscription = bus.subscribe<events::FogConfigChangedEvent>(
        this, &ObjectManager::onFogConfigChanged);

    // Subscribe to performance preset changes
    m_performanceSubscription = bus.subscribe<events::PerformancePresetChangedEvent>(
        this, &ObjectManager::onPerformancePresetChanged);

    std::cout << "[ObjectManager] Subscribed to config events" << std::endl;
}

void ObjectManager::unsubscribeFromEvents()
{
    auto& bus = events::EventBus::instance();

    if (m_fogSubscription != 0)
    {
        bus.unsubscribe(m_fogSubscription);
        m_fogSubscription = 0;
    }

    if (m_performanceSubscription != 0)
    {
        bus.unsubscribe(m_performanceSubscription);
        m_performanceSubscription = 0;
    }
}

void ObjectManager::onFogConfigChanged(const events::FogConfigChangedEvent& event)
{
    // Automatically sync fog settings from ConfigManager
    fogEnabled = event.enabled;
    fogColor = event.color;
    fogDensity = event.density;
    fogDesaturationStrength = event.desaturationStrength;
    fogAbsorptionDensity = event.absorptionDensity;
    fogAbsorptionStrength = event.absorptionStrength;
}

void ObjectManager::onPerformancePresetChanged(const events::PerformancePresetChangedEvent& event)
{
    // Automatically sync performance settings from ConfigManager
    cullingEnabled = event.frustumCullingEnabled;
    lodEnabled = event.lodEnabled;

    // Update systems
    if (m_cullingSystem)
    {
        m_cullingSystem->setCullingEnabled(cullingEnabled);
    }
    if (m_lodSystem)
    {
        m_lodSystem->setLODEnabled(lodEnabled);
    }

    // Reinitialize with new object count if it changed
    if (event.objectCount != static_cast<int>(m_entities.size()) && isInitialized)
    {
        std::cout << "[ObjectManager] Performance preset changed, reinitializing with "
                  << event.objectCount << " objects" << std::endl;
        initialize(event.objectCount);
    }
}
