/**
 * @file LightManager.h
 * @brief Multi-light system with flashlight UBO support
 *
 * LightManager handles all lighting in the scene including directional,
 * point, and spotlight sources. The flashlight is special-cased with its
 * own UBO at binding point 2 for efficient GPU updates.
 *
 * Light Types:
 *   - DIRECTIONAL: Infinite distance, parallel rays (sun)
 *   - POINT: Omnidirectional with attenuation
 *   - SPOTLIGHT: Cone-shaped with cutoff angles
 *
 * Flashlight UBO (binding point 2):
 *   Layout (std140):
 *     vec4 position       (xyz = pos, w = enabled)
 *     vec4 direction      (xyz = dir, w = cutoff cosine)
 *     vec4 colorBrightness(xyz = color, w = brightness)
 *     vec4 params         (x = outerCutoff, yzw = reserved)
 *
 * ECS Integration:
 *   - Creates entities with TransformComponent + LightComponent
 *   - Dual storage: lights vector + ECS entities (for transition)
 *   - m_flashlightEntity tracks the special flashlight
 *
 * Events:
 *   - FlashlightToggleEvent: Toggle on/off
 *   - FlashlightConfigChangedEvent: Update color, brightness, cutoff
 *
 * Usage:
 *   lightManager.initializeGLResources();  // Create UBO
 *   lightManager.setFlashlight(pos, dir);  // Attach to camera
 *   lightManager.applyLightsToShader(program, cameraPos);  // Bind uniforms
 *
 * @see Application, pbr_model.frag, object_instanced.frag
 */
#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <unordered_map>

// ECS includes
#include "ECSWorld.h"
#include "components/TransformComponent.h"
#include "components/LightComponent.h"
#include "events/Events.h"

// Light types
enum class LightType
{
    DIRECTIONAL,
    POINT,
    SPOTLIGHT
};

struct Light
{
    LightType type;
    glm::vec3 position;
    glm::vec3 direction;
    glm::vec3 color;
    float intensity;

    // Spotlight specific
    float cutoff;
    float outerCutoff;

    // Point/Spotlight specific
    float constant;
    float linear;
    float quadratic;

    bool enabled;

    Light() : type(LightType::POINT), position(0.0f), direction(0.0f, 0.0f, -1.0f),
              color(1.0f), intensity(1.0f), cutoff(0.0f), outerCutoff(0.0f),
              constant(1.0f), linear(0.09f), quadratic(0.032f), enabled(true) {}
};

class LightManager
{
public:
    LightManager();
    ~LightManager();

    // Light management
    void addLight(const Light &light);
    void removeLight(size_t index);
    void clearLights();

    // Convenience methods for common lights
    void addDirectionalLight(const glm::vec3 &direction, const glm::vec3 &color = glm::vec3(1.0f), float intensity = 1.0f);
    void addPointLight(const glm::vec3 &position, const glm::vec3 &color = glm::vec3(1.0f), float intensity = 1.0f);
    void addSpotlight(const glm::vec3 &position, const glm::vec3 &direction, float cutoff,
                      const glm::vec3 &color = glm::vec3(1.0f), float intensity = 1.0f);

    // Flashlight management
    void setFlashlight(const glm::vec3 &position, const glm::vec3 &direction, bool enabled = true);
    void updateFlashlight(const glm::vec3 &position, const glm::vec3 &direction);
    void toggleFlashlight();
    bool isFlashlightOn() const { return flashlightEnabled; }
    glm::vec3 getFlashlightPosition() const;
    glm::vec3 getFlashlightDirection() const;
    float getFlashlightCutoff() const;
    float getFlashlightOuterCutoff() const;

    // Flashlight properties
    void setFlashlightBrightness(float brightness);
    void setFlashlightColor(const glm::vec3 &color);
    void setFlashlightCutoff(float cutoffDegrees);
    float getFlashlightBrightness() const;
    glm::vec3 getFlashlightColor() const;

    // Shader interface
    void applyLightsToShader(GLuint shaderProgram, const glm::vec3 &cameraPos) const;

    // GL resources for UBO
    void initializeGLResources();
    void updateFlashlightUBO() const;

    // Getters
    const std::vector<Light> &getLights() const { return lights; }
    size_t getLightCount() const { return lights.size(); }

private:
    std::vector<Light> lights;
    int flashlightIndex; // Changed from size_t to int to allow -1
    bool flashlightEnabled;
    GLuint flashlightUBO = 0; // binding point 2

    // Entity tracking
    std::vector<ecs::Entity> m_lightEntities;
    ecs::Entity m_flashlightEntity;  // Special entity for flashlight

    // ECS helper methods
    ecs::Entity createLightEntity(const Light& light);

    // Event handling
    void onFlashlightToggle(const events::FlashlightToggleEvent& event);
    void onFlashlightConfigChanged(const events::FlashlightConfigChangedEvent& event);
    events::SubscriptionId m_flashlightToggleSubscription = 0;
    events::SubscriptionId m_flashlightConfigSubscription = 0;
    void subscribeToEvents();
    void unsubscribeFromEvents();

    // Cached uniform locations per shader program (avoids glGetUniformLocation every frame)
    static constexpr int MAX_CACHED_LIGHTS = 8;
    struct LightUniformCache {
        GLint numLights = -1;
        GLint viewPos = -1;
        struct PerLight {
            GLint type = -1;
            GLint position = -1;
            GLint direction = -1;
            GLint color = -1;
            GLint intensity = -1;
            GLint cutoff = -1;
            GLint outerCutoff = -1;
            GLint constant = -1;
            GLint linear = -1;
            GLint quadratic = -1;
            GLint enabled = -1;
        } lights[MAX_CACHED_LIGHTS];
    };
    mutable std::unordered_map<GLuint, LightUniformCache> m_uniformCache;
    const LightUniformCache& getCachedUniforms(GLuint shaderProgram) const;
};
