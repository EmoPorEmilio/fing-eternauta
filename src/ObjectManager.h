/**
 * @file ObjectManager.h
 * @brief GPU-instanced rendering of prism objects with LOD and culling
 *
 * ObjectManager handles the rendering of up to 500k+ instanced prisms with
 * a 3-level LOD system and distance-based culling for performance.
 *
 * GPU Resources:
 *   - 3 VAOs/VBOs/EBOs (one per LOD level: HIGH, MEDIUM, LOW)
 *   - 3 instance VBOs for per-LOD model matrices
 *   - Compiled shader program (object_instanced.vert/frag)
 *
 * LOD System:
 *   - HIGH: < 50m from camera (full geometry)
 *   - MEDIUM: 50-150m (reduced geometry)
 *   - LOW: > 150m (minimal geometry)
 *   - Managed by LODSystem which updates LODComponent per entity
 *
 * Culling:
 *   - Distance-based via CullingSystem
 *   - Entities beyond cull distance have RenderableComponent.visible = false
 *
 * ECS Integration:
 *   - Creates entities with TransformComponent, RenderableComponent, LODComponent
 *   - gatherInstanceMatrices() queries ECS for visible entities by LOD level
 *   - Entities stored in m_entities vector for cleanup
 *
 * Performance Presets:
 *   - PRESET_MINIMAL: 3,000 objects
 *   - PRESET_MEDIUM: 15,000 objects
 *   - PRESET_MAXIMUM: 500,000 objects
 *
 * Events:
 *   - FogConfigChangedEvent: Updates fog uniforms
 *   - PerformancePresetChangedEvent: Recreates entities
 *
 * @see LODSystem, CullingSystem, Prism
 */
#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include "Constants.h"
#include "Prism.h"
#include "LightManager.h"
#include "AssetManager.h"
#include "events/Events.h"

// ECS includes
#include "ECSWorld.h"
#include "components/TransformComponent.h"
#include "components/RenderableComponent.h"
#include "components/LODComponent.h"
#include "components/BatchGroupComponent.h"
#include "systems/LODSystem.h"
#include "systems/CullingSystem.h"

class ObjectManager
{
public:
    ObjectManager();
    ~ObjectManager();

    void initialize(int objectCount = PRESET_MAXIMUM);
    void update(const glm::vec3 &cameraPos, float cullDistance, float highLodDistance, float mediumLodDistance, float deltaTime = 0.0f);
    void render(const glm::mat4 &view, const glm::mat4 &projection, const glm::vec3 &cameraPos, const glm::vec3 &cameraFront, const LightManager &lightManager, GLuint textureID);

    void setObjectCount(int count);
    int getObjectCount() const;

    // Performance presets - use Constants::Objects values
    static constexpr int PRESET_MINIMAL = Constants::Objects::PRESET_MINIMAL;
    static constexpr int PRESET_MEDIUM = Constants::Objects::PRESET_MEDIUM;
    static constexpr int PRESET_MAXIMUM = Constants::Objects::PRESET_MAXIMUM;

    // Performance monitoring
    void printPerformanceStats();
    void updatePerformanceStats(float deltaTime);

    // Runtime configuration
    void setCullingEnabled(bool enabled) { cullingEnabled = enabled; }
    void setLODEnabled(bool enabled) { lodEnabled = enabled; }
    bool isCullingEnabled() const { return cullingEnabled; }
    bool isLODEnabled() const { return lodEnabled; }
    void toggleCulling() { cullingEnabled = !cullingEnabled; }
    void toggleLOD() { lodEnabled = !lodEnabled; }

    // Fog configuration - automatically synced via events from ConfigManager
    // Direct setters are still available but prefer ConfigManager for centralized control
    void setFogEnabled(bool enabled) { fogEnabled = enabled; }
    void setFogColor(const glm::vec3 &color) { fogColor = color; }
    void setFogDensity(float density) { fogDensity = density; }
    void setFogDesaturationStrength(float strength) { fogDesaturationStrength = strength; }
    void setFogAbsorption(float density, float strength)
    {
        fogAbsorptionDensity = density;
        fogAbsorptionStrength = strength;
    }

    // Event handlers
    void onFogConfigChanged(const events::FogConfigChangedEvent& event);
    void onPerformancePresetChanged(const events::PerformancePresetChangedEvent& event);

    void cleanup();

private:
    // ECS system pointers (owned by ECSWorld, just cached here for convenience)
    ecs::LODSystem* m_lodSystem = nullptr;
    ecs::CullingSystem* m_cullingSystem = nullptr;

    // Prism geometry template
    Prism prismGeometry;

    // OpenGL objects - multiple VAOs for different LOD levels
    GLuint highLodVao, highLodVbo, highLodEbo;
    GLuint mediumLodVao, mediumLodVbo, mediumLodEbo;
    GLuint lowLodVao, lowLodVbo, lowLodEbo;
    // Instance buffers for model matrices per LOD
    GLuint highLodInstanceVbo;
    GLuint mediumLodInstanceVbo;
    GLuint lowLodInstanceVbo;
    GLuint shaderProgram;

    // LOD geometry data
    Prism highLodPrism, mediumLodPrism, lowLodPrism;

    // Rendering state
    bool isInitialized;

    // Performance monitoring
    float frameTime;
    float fps;
    int frameCount;
    float lastStatsTime;
    static constexpr float STATS_INTERVAL = 1.0f;

    // Runtime configuration
    bool cullingEnabled;
    bool lodEnabled;

    // Fog parameters (will be moved to Config in Phase 3)
    bool fogEnabled;
    glm::vec3 fogColor;
    float fogDensity;
    float fogDesaturationStrength;
    float fogAbsorptionDensity;
    float fogAbsorptionStrength;

    // Constants - use Constants::Objects values
    static constexpr float MIN_DISTANCE = Constants::Objects::MIN_DISTANCE_BETWEEN_OBJECTS;
    static constexpr float WORLD_SIZE = Constants::Objects::WORLD_SIZE;
    static constexpr float HIGH_LOD_DISTANCE = Constants::Objects::HIGH_LOD_DISTANCE;
    static constexpr float MEDIUM_LOD_DISTANCE = Constants::Objects::MEDIUM_LOD_DISTANCE;

    // Setup methods
    void setupLODGeometry();
    void setupShader();
    void setupLODVAO(GLuint &vao, GLuint &vbo, GLuint &ebo, GLuint &instanceVbo, const Prism &prism);

    // Rendering
    void renderLODLevelInstanced(LODLevel lod, const std::vector<glm::mat4> &instanceMatrices);

    // ECS helpers
    void initializeECSSystems();
    void createPrismEntities(int count);
    ecs::Entity createPrismEntity(const glm::vec3& position);
    void gatherInstanceMatrices(std::vector<glm::mat4>& highInstances,
                                std::vector<glm::mat4>& mediumInstances,
                                std::vector<glm::mat4>& lowInstances);

    // Entity tracking for this manager
    std::vector<ecs::Entity> m_entities;

    // Event subscriptions
    events::SubscriptionId m_fogSubscription = 0;
    events::SubscriptionId m_performanceSubscription = 0;
    void subscribeToEvents();
    void unsubscribeFromEvents();

    // Cached uniform locations (avoids glGetUniformLocation every frame)
    struct UniformCache {
        GLint view = -1;
        GLint projection = -1;
        GLint viewPos = -1;
        GLint objectColor = -1;
        GLint useTexture = -1;
        GLint flashlightOn = -1;
        GLint flashlightPos = -1;
        GLint flashlightDir = -1;
        GLint flashlightCutoff = -1;
        GLint flashlightBrightness = -1;
        GLint flashlightColor = -1;
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
