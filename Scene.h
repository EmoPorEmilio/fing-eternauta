#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"
#include "Texture.h"
#include "ObjectManager.h"
#include "LightManager.h"
#include "ModelManager.h"
#include "SnowSystem.h"

class Scene
{
public:
    Scene();
    ~Scene();

    bool initialize();
    void update(const glm::vec3 &cameraPos, float deltaTime = 0.0f, const glm::mat4 &viewMatrix = glm::mat4(1.0f), const glm::mat4 &projectionMatrix = glm::mat4(1.0f));
    void render(const glm::mat4 &view, const glm::mat4 &projection, const glm::vec3 &cameraPos, const glm::vec3 &cameraFront, LightManager &lightManager);
    void cleanup();

    void setObjectCount(int count);
    int getObjectCount() const;

    // Material controls
    void setAmbient(float v) { ambient = v; }
    void setSpecularStrength(float v) { specularStrength = v; }
    void setNormalStrength(float v) { normalStrength = v; }
    void setRoughnessBias(float v) { roughnessBias = v; }
    void setFingPosition(const glm::vec3 &p) { fingPosition = p; }
    void setFingScale(float s) { fingScale = s; }
    glm::vec3 getFingPosition() const { return fingPosition; }

    // Snow system controls
    void setSnowEnabled(bool enabled) { snowSystem.setEnabled(enabled); }
    void setSnowCount(int count) { snowSystem.setCount(count); }
    void setSnowFallSpeed(float speed) { snowSystem.setFallSpeed(speed); }
    void setSnowWindSpeed(float speed) { snowSystem.setWindSpeed(speed); }
    void setSnowWindDirection(float yawDegrees) { snowSystem.setWindDirection(yawDegrees); }
    void setSnowSpriteSize(float size) { snowSystem.setSpriteSize(size); }
    void setSnowTimeScale(float scale) { snowSystem.setTimeScale(scale); }
    void setSnowBulletGroundCollision(bool enabled) { snowSystem.setBulletGroundCollisionEnabled(enabled); }

    // Snow performance controls
    void setSnowFrustumCulling(bool enabled) { snowSystem.setFrustumCulling(enabled); }
    void setSnowLOD(bool) {}
    void setSnowMaxVisible(int) {}

    // Runtime configuration
    void toggleCulling() { objectManager.toggleCulling(); }
    void toggleLOD() { objectManager.toggleLOD(); }
    bool isCullingEnabled() const { return objectManager.isCullingEnabled(); }
    bool isLODEnabled() const { return objectManager.isLODEnabled(); }
    void setObjectCulling(bool enabled) { objectManager.setCullingEnabled(enabled); }
    void setObjectLOD(bool enabled) { objectManager.setLODEnabled(enabled); }

    // Fog controls
    void setFogEnabled(bool enabled) { fogEnabled = enabled; }
    void setFogColor(const glm::vec3 &color) { fogColor = color; }
    void setFogDensity(float density) { fogDensity = density; }
    void setFogDesaturationStrength(float strength) { fogDesaturationStrength = strength; }
    void setFogAbsorption(float density, float strength)
    {
        fogAbsorptionDensity = density;
        fogAbsorptionStrength = strength;
    }

private:
    // Floor plane
    GLuint vao, vbo;
    Shader shader;
    Texture albedoTex;
    Texture roughnessTex;
    Texture translucencyTex;
    Texture heightTex;

    // Object management
    ObjectManager objectManager;

    // GLTF Model management
    ModelManager modelManager;
    int fingInstanceId = -1;
    int militaryInstanceId = -1;
    int walkingInstanceId = -1;

    // Snow system
    SnowSystem snowSystem;

    // Rendering parameters
    float cullDistance;
    float lodDistance;

    // Runtime-tunable material params
    float ambient = 0.2f;
    float specularStrength = 0.5f;
    float normalStrength = 0.276f;
    float roughnessBias = 0.0f;

    // FING model transform controls
    glm::vec3 fingPosition = glm::vec3(0.0f, 119.900f, -222.300f);
    float fingScale = 21.3f;

    // MILITARY model transform controls
    glm::vec3 militaryPosition = glm::vec3(0.0f, 0.0f, -100.0f);
    float militaryScale = 8.5f;

    // WALKING model transform controls
    glm::vec3 walkingPosition = glm::vec3(50.0f, 0.0f, -50.0f);
    float walkingScale = 5.0f;

    // Animation clock
    float animElapsed = 0.0f;
    bool militaryAnimEnabled = true;
    float militaryAnimSpeed = 1.0f;
    bool walkingAnimEnabled = true;
    float walkingAnimSpeed = 1.0f;

    // Fog parameters
    bool fogEnabled = true;
    glm::vec3 fogColor = glm::vec3(0.1098f, 0.1255f, 0.1490f); // #1C2026
    float fogDensity = 0.0107f;
    float fogDesaturationStrength = 0.48f;
    float fogAbsorptionDensity = 0.02f;
    float fogAbsorptionStrength = 0.8f;

    void setupGeometry();
    void setupShader();

public:
    // Military controls
    void setMilitaryPosition(const glm::vec3 &p) { militaryPosition = p; }
    void setMilitaryScale(float s) { militaryScale = s; }
    void setMilitaryAnimEnabled(bool e) { militaryAnimEnabled = e; }
    void setMilitaryAnimSpeed(float s) { militaryAnimSpeed = s; }

    // Walking model controls
    void setWalkingPosition(const glm::vec3 &p) { walkingPosition = p; }
    void setWalkingScale(float s) { walkingScale = s; }
    void setWalkingAnimEnabled(bool e) { walkingAnimEnabled = e; }
    void setWalkingAnimSpeed(float s) { walkingAnimSpeed = s; }
};
