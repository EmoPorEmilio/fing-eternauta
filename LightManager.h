#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>

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
};
