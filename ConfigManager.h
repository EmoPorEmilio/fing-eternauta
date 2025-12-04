/**
 * @file ConfigManager.h
 * @brief Centralized configuration with event-driven updates
 *
 * ConfigManager is the single source of truth for all runtime settings.
 * When settings change, it publishes typed events that managers subscribe to,
 * enabling decoupled updates across the system.
 *
 * Configuration Sections:
 *   - FogConfig: Exponential fog with desaturation and absorption
 *   - LightingConfig: Ambient and specular settings
 *   - FlashlightConfig: Color, brightness, cutoff
 *   - PerformanceConfig: Object count, culling, LOD presets
 *   - SnowConfig: Particle count, speed, wind
 *   - MaterialConfig: Surface properties
 *   - DebugVisualsConfig: Grid, axes, gizmo visibility
 *   - CameraConfig: FOV, near/far planes, movement speed
 *
 * Event Publishing:
 *   Each setter method publishes a corresponding event:
 *   - setFogEnabled() -> FogConfigChangedEvent
 *   - setFlashlightBrightness() -> FlashlightConfigChangedEvent
 *   - setPerformancePreset() -> PerformancePresetChangedEvent
 *   - etc.
 *
 * Usage Pattern:
 *   // Update setting (automatically publishes event)
 *   ConfigManager::instance().setFogDensity(0.01f);
 *
 *   // Subscribe to changes in other classes
 *   EventBus::subscribe<FogConfigChangedEvent>([](auto& e) {
 *       updateFogUniforms(e.density, e.color);
 *   });
 *
 * Persistence:
 *   saveToFile() / loadFromFile() - Not yet implemented
 *
 * Known Issues:
 *   - UIManager duplicates some state (should read from ConfigManager instead)
 *   - Managers also duplicate fog parameters (should query ConfigManager)
 *
 * @see events/ConfigEvents.h, UIManager
 */
#pragma once

#include <glm/glm.hpp>
#include "events/Events.h"

class ConfigManager {
public:
    // Fog configuration
    struct FogConfig {
        bool enabled = true;
        glm::vec3 color = glm::vec3(0.0f, 0.0f, 0.0f);
        float density = 0.005f;
        float desaturationStrength = 0.79f;
        float absorptionDensity = 0.02f;
        float absorptionStrength = 0.8f;
    };

    // Lighting configuration
    struct LightingConfig {
        glm::vec3 ambientColor = glm::vec3(0.1f);
        float ambientIntensity = 0.1f;
        float specularStrength = 0.5f;
        float shininess = 32.0f;
    };

    // Flashlight configuration
    struct FlashlightConfig {
        bool enabled = false;
        glm::vec3 color = glm::vec3(1.0f, 0.8f, 0.6f);
        float brightness = 3.0f;
        float cutoffDegrees = 25.0f;
    };

    // Performance configuration
    struct PerformanceConfig {
        enum class Preset { Low, Medium, High, Ultra, Custom };
        Preset preset = Preset::Medium;
        int objectCount = 100000;
        bool frustumCullingEnabled = true;
        bool lodEnabled = true;
    };

    // Snow configuration
    struct SnowConfig {
        bool enabled = true;
        int count = 30000;
        float fallSpeed = 10.0f;
        float windSpeed = 5.0f;
        float windDirection = 180.0f;  // Degrees
        float spriteSize = 0.05f;
        float timeScale = 1.0f;
        bool bulletCollisionEnabled = false;
    };

    // Material configuration (surface properties)
    struct MaterialConfig {
        float ambient = 0.2f;
        float specularStrength = 0.5f;
        float normalStrength = 0.276f;
        float roughnessBias = 0.0f;
    };

    // Debug configuration
    struct DebugConfig {
        bool showGrid = false;
        bool showOriginAxes = false;
        bool showNormals = false;
        bool wireframeMode = false;
    };

    // Debug visuals configuration (extended for viewport)
    struct DebugVisualsConfig {
        bool showGrid = true;
        bool showOriginAxes = true;
        bool showGizmo = true;
        float gridScale = 1.0f;
        float gridFadeDistance = 150.0f;
        int floorMode = 0;  // 0=GridOnly, 1=TexturedSnow, 2=Both
    };

    // Camera configuration
    struct CameraConfig {
        float fov = 60.0f;
        float nearPlane = 0.1f;
        float farPlane = 500.0f;
        float moveSpeed = 10.0f;
        float mouseSensitivity = 0.1f;
    };

    // Model instance configuration
    struct ModelInstanceConfig {
        bool enabled = true;
        glm::vec3 position = glm::vec3(0.0f);
        float scale = 1000.0f;
        bool animationEnabled = true;
        float animationSpeed = 1.0f;
    };

    // Models configuration (scene models)
    struct ModelsConfig {
        ModelInstanceConfig walking;   // model_Animation_Walking_withSkin.glb
        ModelInstanceConfig monster2;  // monster-2.glb
    };

    // Singleton access
    static ConfigManager& instance();

    // Fog getters/setters
    const FogConfig& getFog() const { return m_fog; }
    void setFog(const FogConfig& config);
    void setFogEnabled(bool enabled);
    void setFogColor(const glm::vec3& color);
    void setFogDensity(float density);
    void setFogDesaturationStrength(float strength);
    void setFogAbsorption(float density, float strength);

    // Lighting getters/setters
    const LightingConfig& getLighting() const { return m_lighting; }
    void setLighting(const LightingConfig& config);
    void setAmbient(const glm::vec3& color, float intensity);
    void setSpecular(float strength, float shininess);

    // Flashlight getters/setters
    const FlashlightConfig& getFlashlight() const { return m_flashlight; }
    void setFlashlight(const FlashlightConfig& config);
    void setFlashlightEnabled(bool enabled);
    void setFlashlightColor(const glm::vec3& color);
    void setFlashlightBrightness(float brightness);
    void setFlashlightCutoff(float degrees);

    // Performance getters/setters
    const PerformanceConfig& getPerformance() const { return m_performance; }
    void setPerformance(const PerformanceConfig& config);
    void setPerformancePreset(PerformanceConfig::Preset preset);
    void setObjectCount(int count);
    void setFrustumCullingEnabled(bool enabled);
    void setLodEnabled(bool enabled);

    // Snow getters/setters
    const SnowConfig& getSnow() const { return m_snow; }
    void setSnow(const SnowConfig& config);
    void setSnowEnabled(bool enabled);
    void setSnowCount(int count);
    void setSnowFallSpeed(float speed);
    void setSnowWind(float speed, float direction);
    void setSnowSpriteSize(float size);
    void setSnowTimeScale(float scale);
    void setSnowBulletCollision(bool enabled);

    // Material getters/setters
    const MaterialConfig& getMaterial() const { return m_material; }
    void setMaterial(const MaterialConfig& config);
    void setMaterialAmbient(float ambient);
    void setMaterialSpecular(float strength);
    void setMaterialNormal(float strength);
    void setMaterialRoughnessBias(float bias);

    // Debug getters/setters
    const DebugConfig& getDebug() const { return m_debug; }
    void setDebug(const DebugConfig& config);

    // Debug visuals getters/setters (extended)
    const DebugVisualsConfig& getDebugVisuals() const { return m_debugVisuals; }
    void setDebugVisuals(const DebugVisualsConfig& config);
    void setDebugVisualsGrid(bool show, float scale, float fadeDistance);
    void setDebugVisualsAxes(bool show);
    void setDebugVisualsGizmo(bool show);
    void setDebugVisualsFloorMode(int mode);

    // Camera getters/setters
    const CameraConfig& getCamera() const { return m_camera; }
    void setCamera(const CameraConfig& config);

    // Models getters/setters
    const ModelsConfig& getModels() const { return m_models; }
    void setModels(const ModelsConfig& config);
    void setModelWalking(const ModelInstanceConfig& config);
    void setModelMonster2(const ModelInstanceConfig& config);

    // Persistence (future use)
    bool saveToFile(const std::string& filepath);
    bool loadFromFile(const std::string& filepath);

private:
    ConfigManager() = default;
    ~ConfigManager() = default;
    ConfigManager(const ConfigManager&) = delete;
    ConfigManager& operator=(const ConfigManager&) = delete;

    // Publish helpers
    void publishFogChanged();
    void publishLightingChanged();
    void publishFlashlightChanged();
    void publishPerformanceChanged();
    void publishSnowChanged();
    void publishDebugChanged();
    void publishCameraChanged();
    void publishMaterialChanged();
    void publishDebugVisualsChanged();
    void publishModelsChanged();

    FogConfig m_fog;
    LightingConfig m_lighting;
    FlashlightConfig m_flashlight;
    PerformanceConfig m_performance;
    SnowConfig m_snow;
    MaterialConfig m_material;
    DebugConfig m_debug;
    DebugVisualsConfig m_debugVisuals;
    CameraConfig m_camera;
    ModelsConfig m_models;
};
