#include "ConfigManager.h"
#include <fstream>
#include <iostream>
#include "libraries/tinygltf/json.hpp"

using json = nlohmann::json;

ConfigManager& ConfigManager::instance() {
    static ConfigManager manager;
    return manager;
}

// ==================== Fog ====================

void ConfigManager::setFog(const FogConfig& config) {
    m_fog = config;
    publishFogChanged();
}

void ConfigManager::setFogEnabled(bool enabled) {
    if (m_fog.enabled != enabled) {
        m_fog.enabled = enabled;
        publishFogChanged();
    }
}

void ConfigManager::setFogColor(const glm::vec3& color) {
    if (m_fog.color != color) {
        m_fog.color = color;
        publishFogChanged();
    }
}

void ConfigManager::setFogDensity(float density) {
    if (m_fog.density != density) {
        m_fog.density = density;
        publishFogChanged();
    }
}

void ConfigManager::setFogDesaturationStrength(float strength) {
    if (m_fog.desaturationStrength != strength) {
        m_fog.desaturationStrength = strength;
        publishFogChanged();
    }
}

void ConfigManager::setFogAbsorption(float density, float strength) {
    if (m_fog.absorptionDensity != density || m_fog.absorptionStrength != strength) {
        m_fog.absorptionDensity = density;
        m_fog.absorptionStrength = strength;
        publishFogChanged();
    }
}

void ConfigManager::publishFogChanged() {
    events::EventBus::instance().publish(events::FogConfigChangedEvent(
        m_fog.enabled, m_fog.color, m_fog.density, m_fog.desaturationStrength,
        m_fog.absorptionDensity, m_fog.absorptionStrength
    ));
}

// ==================== Lighting ====================

void ConfigManager::setLighting(const LightingConfig& config) {
    m_lighting = config;
    publishLightingChanged();
}

void ConfigManager::setAmbient(const glm::vec3& color, float intensity) {
    if (m_lighting.ambientColor != color || m_lighting.ambientIntensity != intensity) {
        m_lighting.ambientColor = color;
        m_lighting.ambientIntensity = intensity;
        publishLightingChanged();
    }
}

void ConfigManager::setSpecular(float strength, float shininess) {
    if (m_lighting.specularStrength != strength || m_lighting.shininess != shininess) {
        m_lighting.specularStrength = strength;
        m_lighting.shininess = shininess;
        publishLightingChanged();
    }
}

void ConfigManager::publishLightingChanged() {
    events::EventBus::instance().publish(events::LightingConfigChangedEvent(
        m_lighting.ambientColor, m_lighting.ambientIntensity,
        m_lighting.specularStrength, m_lighting.shininess
    ));
}

// ==================== Flashlight ====================

void ConfigManager::setFlashlight(const FlashlightConfig& config) {
    m_flashlight = config;
    publishFlashlightChanged();
}

void ConfigManager::setFlashlightEnabled(bool enabled) {
    if (m_flashlight.enabled != enabled) {
        m_flashlight.enabled = enabled;
        publishFlashlightChanged();
    }
}

void ConfigManager::setFlashlightColor(const glm::vec3& color) {
    if (m_flashlight.color != color) {
        m_flashlight.color = color;
        publishFlashlightChanged();
    }
}

void ConfigManager::setFlashlightBrightness(float brightness) {
    if (m_flashlight.brightness != brightness) {
        m_flashlight.brightness = brightness;
        publishFlashlightChanged();
    }
}

void ConfigManager::setFlashlightCutoff(float degrees) {
    if (m_flashlight.cutoffDegrees != degrees) {
        m_flashlight.cutoffDegrees = degrees;
        publishFlashlightChanged();
    }
}

void ConfigManager::publishFlashlightChanged() {
    events::EventBus::instance().publish(events::FlashlightConfigChangedEvent(
        m_flashlight.enabled, m_flashlight.color,
        m_flashlight.brightness, m_flashlight.cutoffDegrees
    ));
}

// ==================== Performance ====================

void ConfigManager::setPerformance(const PerformanceConfig& config) {
    m_performance = config;
    publishPerformanceChanged();
}

void ConfigManager::setPerformancePreset(PerformanceConfig::Preset preset) {
    m_performance.preset = preset;

    // Set defaults based on preset
    switch (preset) {
        case PerformanceConfig::Preset::Low:
            m_performance.objectCount = 10000;
            m_performance.frustumCullingEnabled = true;
            m_performance.lodEnabled = true;
            break;
        case PerformanceConfig::Preset::Medium:
            m_performance.objectCount = 100000;
            m_performance.frustumCullingEnabled = true;
            m_performance.lodEnabled = true;
            break;
        case PerformanceConfig::Preset::High:
            m_performance.objectCount = 250000;
            m_performance.frustumCullingEnabled = true;
            m_performance.lodEnabled = true;
            break;
        case PerformanceConfig::Preset::Ultra:
            m_performance.objectCount = 500000;
            m_performance.frustumCullingEnabled = true;
            m_performance.lodEnabled = true;
            break;
        case PerformanceConfig::Preset::Custom:
            // Don't change other settings for Custom
            break;
    }

    publishPerformanceChanged();
}

void ConfigManager::setObjectCount(int count) {
    if (m_performance.objectCount != count) {
        m_performance.objectCount = count;
        m_performance.preset = PerformanceConfig::Preset::Custom;
        publishPerformanceChanged();
    }
}

void ConfigManager::setFrustumCullingEnabled(bool enabled) {
    if (m_performance.frustumCullingEnabled != enabled) {
        m_performance.frustumCullingEnabled = enabled;
        publishPerformanceChanged();
    }
}

void ConfigManager::setLodEnabled(bool enabled) {
    if (m_performance.lodEnabled != enabled) {
        m_performance.lodEnabled = enabled;
        publishPerformanceChanged();
    }
}

void ConfigManager::publishPerformanceChanged() {
    events::EventBus::instance().publish(events::PerformancePresetChangedEvent(
        static_cast<events::PerformancePresetChangedEvent::Preset>(m_performance.preset),
        m_performance.objectCount,
        m_performance.frustumCullingEnabled,
        m_performance.lodEnabled
    ));
}

// ==================== Snow ====================

void ConfigManager::setSnow(const SnowConfig& config) {
    m_snow = config;
    publishSnowChanged();
}

void ConfigManager::setSnowEnabled(bool enabled) {
    if (m_snow.enabled != enabled) {
        m_snow.enabled = enabled;
        publishSnowChanged();
    }
}

void ConfigManager::setSnowCount(int count) {
    if (m_snow.count != count) {
        m_snow.count = count;
        publishSnowChanged();
    }
}

void ConfigManager::setSnowFallSpeed(float speed) {
    if (m_snow.fallSpeed != speed) {
        m_snow.fallSpeed = speed;
        publishSnowChanged();
    }
}

void ConfigManager::setSnowWind(float speed, float direction) {
    if (m_snow.windSpeed != speed || m_snow.windDirection != direction) {
        m_snow.windSpeed = speed;
        m_snow.windDirection = direction;
        publishSnowChanged();
    }
}

void ConfigManager::setSnowSpriteSize(float size) {
    if (m_snow.spriteSize != size) {
        m_snow.spriteSize = size;
        publishSnowChanged();
    }
}

void ConfigManager::setSnowTimeScale(float scale) {
    if (m_snow.timeScale != scale) {
        m_snow.timeScale = scale;
        publishSnowChanged();
    }
}

void ConfigManager::setSnowBulletCollision(bool enabled) {
    if (m_snow.bulletCollisionEnabled != enabled) {
        m_snow.bulletCollisionEnabled = enabled;
        publishSnowChanged();
    }
}

void ConfigManager::publishSnowChanged() {
    events::EventBus::instance().publish(events::SnowConfigChangedEvent(
        m_snow.enabled, m_snow.count, m_snow.fallSpeed, m_snow.windSpeed,
        m_snow.windDirection, m_snow.spriteSize, m_snow.timeScale,
        m_snow.bulletCollisionEnabled
    ));
}

// ==================== Debug ====================

void ConfigManager::setDebug(const DebugConfig& config) {
    m_debug = config;
    publishDebugChanged();
}

void ConfigManager::publishDebugChanged() {
    events::EventBus::instance().publish(events::DebugConfigChangedEvent(
        m_debug.showGrid, m_debug.showOriginAxes,
        m_debug.showNormals, m_debug.wireframeMode
    ));
}

// ==================== Camera ====================

void ConfigManager::setCamera(const CameraConfig& config) {
    m_camera = config;
    publishCameraChanged();
}

void ConfigManager::publishCameraChanged() {
    events::EventBus::instance().publish(events::CameraConfigChangedEvent(
        m_camera.fov, m_camera.nearPlane, m_camera.farPlane,
        m_camera.moveSpeed, m_camera.mouseSensitivity
    ));
}

// ==================== Material ====================

void ConfigManager::setMaterial(const MaterialConfig& config) {
    m_material = config;
    publishMaterialChanged();
}

void ConfigManager::setMaterialAmbient(float ambient) {
    if (m_material.ambient != ambient) {
        m_material.ambient = ambient;
        publishMaterialChanged();
    }
}

void ConfigManager::setMaterialSpecular(float strength) {
    if (m_material.specularStrength != strength) {
        m_material.specularStrength = strength;
        publishMaterialChanged();
    }
}

void ConfigManager::setMaterialNormal(float strength) {
    if (m_material.normalStrength != strength) {
        m_material.normalStrength = strength;
        publishMaterialChanged();
    }
}

void ConfigManager::setMaterialRoughnessBias(float bias) {
    if (m_material.roughnessBias != bias) {
        m_material.roughnessBias = bias;
        publishMaterialChanged();
    }
}

void ConfigManager::publishMaterialChanged() {
    events::EventBus::instance().publish(events::MaterialConfigChangedEvent(
        m_material.ambient, m_material.specularStrength,
        m_material.normalStrength, m_material.roughnessBias
    ));
}

// ==================== Debug Visuals ====================

void ConfigManager::setDebugVisuals(const DebugVisualsConfig& config) {
    m_debugVisuals = config;
    publishDebugVisualsChanged();
}

void ConfigManager::setDebugVisualsGrid(bool show, float scale, float fadeDistance) {
    bool changed = m_debugVisuals.showGrid != show ||
                   m_debugVisuals.gridScale != scale ||
                   m_debugVisuals.gridFadeDistance != fadeDistance;
    if (changed) {
        m_debugVisuals.showGrid = show;
        m_debugVisuals.gridScale = scale;
        m_debugVisuals.gridFadeDistance = fadeDistance;
        publishDebugVisualsChanged();
    }
}

void ConfigManager::setDebugVisualsAxes(bool show) {
    if (m_debugVisuals.showOriginAxes != show) {
        m_debugVisuals.showOriginAxes = show;
        publishDebugVisualsChanged();
    }
}

void ConfigManager::setDebugVisualsGizmo(bool show) {
    if (m_debugVisuals.showGizmo != show) {
        m_debugVisuals.showGizmo = show;
        publishDebugVisualsChanged();
    }
}

void ConfigManager::setDebugVisualsFloorMode(int mode) {
    if (m_debugVisuals.floorMode != mode) {
        m_debugVisuals.floorMode = mode;
        publishDebugVisualsChanged();
    }
}

void ConfigManager::publishDebugVisualsChanged() {
    events::EventBus::instance().publish(events::DebugVisualsChangedEvent(
        m_debugVisuals.showGrid, m_debugVisuals.showOriginAxes, m_debugVisuals.showGizmo,
        m_debugVisuals.gridScale, m_debugVisuals.gridFadeDistance, m_debugVisuals.floorMode
    ));
}

// ==================== Persistence ====================

bool ConfigManager::saveToFile(const std::string& filepath) {
    try {
        json j;

        // Fog
        j["fog"] = {
            {"enabled", m_fog.enabled},
            {"color", {m_fog.color.r, m_fog.color.g, m_fog.color.b}},
            {"density", m_fog.density},
            {"desaturationStrength", m_fog.desaturationStrength},
            {"absorptionDensity", m_fog.absorptionDensity},
            {"absorptionStrength", m_fog.absorptionStrength}
        };

        // Lighting
        j["lighting"] = {
            {"ambientColor", {m_lighting.ambientColor.r, m_lighting.ambientColor.g, m_lighting.ambientColor.b}},
            {"ambientIntensity", m_lighting.ambientIntensity},
            {"specularStrength", m_lighting.specularStrength},
            {"shininess", m_lighting.shininess}
        };

        // Flashlight
        j["flashlight"] = {
            {"enabled", m_flashlight.enabled},
            {"color", {m_flashlight.color.r, m_flashlight.color.g, m_flashlight.color.b}},
            {"brightness", m_flashlight.brightness},
            {"cutoffDegrees", m_flashlight.cutoffDegrees}
        };

        // Performance
        j["performance"] = {
            {"preset", static_cast<int>(m_performance.preset)},
            {"objectCount", m_performance.objectCount},
            {"frustumCullingEnabled", m_performance.frustumCullingEnabled},
            {"lodEnabled", m_performance.lodEnabled}
        };

        // Snow
        j["snow"] = {
            {"enabled", m_snow.enabled},
            {"count", m_snow.count},
            {"fallSpeed", m_snow.fallSpeed},
            {"windSpeed", m_snow.windSpeed},
            {"windDirection", m_snow.windDirection},
            {"spriteSize", m_snow.spriteSize},
            {"timeScale", m_snow.timeScale},
            {"bulletCollisionEnabled", m_snow.bulletCollisionEnabled}
        };

        // Material
        j["material"] = {
            {"ambient", m_material.ambient},
            {"specularStrength", m_material.specularStrength},
            {"normalStrength", m_material.normalStrength},
            {"roughnessBias", m_material.roughnessBias}
        };

        // Debug visuals
        j["debugVisuals"] = {
            {"showGrid", m_debugVisuals.showGrid},
            {"showOriginAxes", m_debugVisuals.showOriginAxes},
            {"showGizmo", m_debugVisuals.showGizmo},
            {"gridScale", m_debugVisuals.gridScale},
            {"gridFadeDistance", m_debugVisuals.gridFadeDistance},
            {"floorMode", m_debugVisuals.floorMode}
        };

        // Camera
        j["camera"] = {
            {"fov", m_camera.fov},
            {"nearPlane", m_camera.nearPlane},
            {"farPlane", m_camera.farPlane},
            {"moveSpeed", m_camera.moveSpeed},
            {"mouseSensitivity", m_camera.mouseSensitivity}
        };

        std::ofstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[ConfigManager] Failed to open file for writing: " << filepath << std::endl;
            return false;
        }

        file << j.dump(2);  // Pretty print with 2-space indent
        std::cout << "[ConfigManager] Configuration saved to: " << filepath << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "[ConfigManager] Error saving config: " << e.what() << std::endl;
        return false;
    }
}

bool ConfigManager::loadFromFile(const std::string& filepath) {
    try {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "[ConfigManager] Failed to open file for reading: " << filepath << std::endl;
            return false;
        }

        json j;
        file >> j;

        // Helper lambda to safely get values
        auto getVec3 = [](const json& arr) -> glm::vec3 {
            return glm::vec3(arr[0].get<float>(), arr[1].get<float>(), arr[2].get<float>());
        };

        // Fog
        if (j.contains("fog")) {
            const auto& fog = j["fog"];
            m_fog.enabled = fog.value("enabled", m_fog.enabled);
            if (fog.contains("color")) m_fog.color = getVec3(fog["color"]);
            m_fog.density = fog.value("density", m_fog.density);
            m_fog.desaturationStrength = fog.value("desaturationStrength", m_fog.desaturationStrength);
            m_fog.absorptionDensity = fog.value("absorptionDensity", m_fog.absorptionDensity);
            m_fog.absorptionStrength = fog.value("absorptionStrength", m_fog.absorptionStrength);
        }

        // Lighting
        if (j.contains("lighting")) {
            const auto& lighting = j["lighting"];
            if (lighting.contains("ambientColor")) m_lighting.ambientColor = getVec3(lighting["ambientColor"]);
            m_lighting.ambientIntensity = lighting.value("ambientIntensity", m_lighting.ambientIntensity);
            m_lighting.specularStrength = lighting.value("specularStrength", m_lighting.specularStrength);
            m_lighting.shininess = lighting.value("shininess", m_lighting.shininess);
        }

        // Flashlight
        if (j.contains("flashlight")) {
            const auto& fl = j["flashlight"];
            m_flashlight.enabled = fl.value("enabled", m_flashlight.enabled);
            if (fl.contains("color")) m_flashlight.color = getVec3(fl["color"]);
            m_flashlight.brightness = fl.value("brightness", m_flashlight.brightness);
            m_flashlight.cutoffDegrees = fl.value("cutoffDegrees", m_flashlight.cutoffDegrees);
        }

        // Performance
        if (j.contains("performance")) {
            const auto& perf = j["performance"];
            m_performance.preset = static_cast<PerformanceConfig::Preset>(perf.value("preset", static_cast<int>(m_performance.preset)));
            m_performance.objectCount = perf.value("objectCount", m_performance.objectCount);
            m_performance.frustumCullingEnabled = perf.value("frustumCullingEnabled", m_performance.frustumCullingEnabled);
            m_performance.lodEnabled = perf.value("lodEnabled", m_performance.lodEnabled);
        }

        // Snow
        if (j.contains("snow")) {
            const auto& snow = j["snow"];
            m_snow.enabled = snow.value("enabled", m_snow.enabled);
            m_snow.count = snow.value("count", m_snow.count);
            m_snow.fallSpeed = snow.value("fallSpeed", m_snow.fallSpeed);
            m_snow.windSpeed = snow.value("windSpeed", m_snow.windSpeed);
            m_snow.windDirection = snow.value("windDirection", m_snow.windDirection);
            m_snow.spriteSize = snow.value("spriteSize", m_snow.spriteSize);
            m_snow.timeScale = snow.value("timeScale", m_snow.timeScale);
            m_snow.bulletCollisionEnabled = snow.value("bulletCollisionEnabled", m_snow.bulletCollisionEnabled);
        }

        // Material
        if (j.contains("material")) {
            const auto& mat = j["material"];
            m_material.ambient = mat.value("ambient", m_material.ambient);
            m_material.specularStrength = mat.value("specularStrength", m_material.specularStrength);
            m_material.normalStrength = mat.value("normalStrength", m_material.normalStrength);
            m_material.roughnessBias = mat.value("roughnessBias", m_material.roughnessBias);
        }

        // Debug visuals
        if (j.contains("debugVisuals")) {
            const auto& dv = j["debugVisuals"];
            m_debugVisuals.showGrid = dv.value("showGrid", m_debugVisuals.showGrid);
            m_debugVisuals.showOriginAxes = dv.value("showOriginAxes", m_debugVisuals.showOriginAxes);
            m_debugVisuals.showGizmo = dv.value("showGizmo", m_debugVisuals.showGizmo);
            m_debugVisuals.gridScale = dv.value("gridScale", m_debugVisuals.gridScale);
            m_debugVisuals.gridFadeDistance = dv.value("gridFadeDistance", m_debugVisuals.gridFadeDistance);
            m_debugVisuals.floorMode = dv.value("floorMode", m_debugVisuals.floorMode);
        }

        // Camera
        if (j.contains("camera")) {
            const auto& cam = j["camera"];
            m_camera.fov = cam.value("fov", m_camera.fov);
            m_camera.nearPlane = cam.value("nearPlane", m_camera.nearPlane);
            m_camera.farPlane = cam.value("farPlane", m_camera.farPlane);
            m_camera.moveSpeed = cam.value("moveSpeed", m_camera.moveSpeed);
            m_camera.mouseSensitivity = cam.value("mouseSensitivity", m_camera.mouseSensitivity);
        }

        // Publish all changes
        publishFogChanged();
        publishLightingChanged();
        publishFlashlightChanged();
        publishPerformanceChanged();
        publishSnowChanged();
        publishMaterialChanged();
        publishDebugVisualsChanged();
        publishCameraChanged();

        std::cout << "[ConfigManager] Configuration loaded from: " << filepath << std::endl;
        return true;

    } catch (const std::exception& e) {
        std::cerr << "[ConfigManager] Error loading config: " << e.what() << std::endl;
        return false;
    }
}
