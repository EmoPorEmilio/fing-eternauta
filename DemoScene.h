#pragma once

#include "BaseScene.h"
#include "ObjectManager.h"
#include "ModelManager.h"
#include "SnowSystem.h"

/**
 * @brief Full demo scene with all content
 *
 * This is the original full scene containing:
 * - Instanced prism objects (ObjectManager)
 * - GLTF models (ModelManager)
 * - Snow particle system (SnowSystem)
 *
 * Inherits floor and fog from BaseScene.
 */
class DemoScene : public BaseScene
{
public:
    DemoScene();
    ~DemoScene() override;

    // IScene interface
    bool initialize() override;
    void update(const glm::vec3& cameraPos, float deltaTime,
               const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) override;
    void render(const glm::mat4& view, const glm::mat4& projection,
               const glm::vec3& cameraPos, const glm::vec3& cameraFront,
               LightManager& lightManager) override;
    void cleanup() override;

    // Object count controls
    void setObjectCount(int count);
    int getObjectCount() const;

    // Model controls
    void setFingPosition(const glm::vec3& p) { m_fingPosition = p; }
    void setFingScale(float s) { m_fingScale = s; }
    glm::vec3 getFingPosition() const { return m_fingPosition; }

    // Military controls
    void setMilitaryPosition(const glm::vec3& p) { m_militaryPosition = p; }
    void setMilitaryScale(float s) { m_militaryScale = s; }
    void setMilitaryAnimEnabled(bool e) { m_militaryAnimEnabled = e; }
    void setMilitaryAnimSpeed(float s) { m_militaryAnimSpeed = s; }

    // Walking model controls
    void setWalkingPosition(const glm::vec3& p) { m_walkingPosition = p; }
    void setWalkingScale(float s) { m_walkingScale = s; }
    void setWalkingAnimEnabled(bool e) { m_walkingAnimEnabled = e; }
    void setWalkingAnimSpeed(float s) { m_walkingAnimSpeed = s; }

    // Snow system controls
    void setSnowEnabled(bool enabled) { m_snowSystem.setEnabled(enabled); }
    void setSnowCount(int count) { m_snowSystem.setCount(count); }
    void setSnowFallSpeed(float speed) { m_snowSystem.setFallSpeed(speed); }
    void setSnowWindSpeed(float speed) { m_snowSystem.setWindSpeed(speed); }
    void setSnowWindDirection(float yawDegrees) { m_snowSystem.setWindDirection(yawDegrees); }
    void setSnowSpriteSize(float size) { m_snowSystem.setSpriteSize(size); }
    void setSnowTimeScale(float scale) { m_snowSystem.setTimeScale(scale); }
    void setSnowBulletGroundCollision(bool enabled) { m_snowSystem.setBulletGroundCollisionEnabled(enabled); }
    void setSnowFrustumCulling(bool enabled) { m_snowSystem.setFrustumCulling(enabled); }
    void setSnowLOD(bool) {} // Placeholder
    void setSnowMaxVisible(int) {} // Placeholder

    // Runtime configuration
    void toggleCulling() { m_objectManager.toggleCulling(); }
    void toggleLOD() { m_objectManager.toggleLOD(); }
    bool isCullingEnabled() const { return m_objectManager.isCullingEnabled(); }
    bool isLODEnabled() const { return m_objectManager.isLODEnabled(); }
    void setObjectCulling(bool enabled) { m_objectManager.setCullingEnabled(enabled); }
    void setObjectLOD(bool enabled) { m_objectManager.setLODEnabled(enabled); }

private:
    // Object management
    ObjectManager m_objectManager;

    // GLTF Model management
    ModelManager m_modelManager;
    int m_fingInstanceId = -1;
    int m_militaryInstanceId = -1;
    int m_walkingInstanceId = -1;

    // Snow system
    SnowSystem m_snowSystem;

    // Rendering parameters
    float m_cullDistance = 200.0f;

    // FING model transform controls
    glm::vec3 m_fingPosition = glm::vec3(0.0f, 119.900f, -222.300f);
    float m_fingScale = 21.3f;

    // MILITARY model transform controls
    glm::vec3 m_militaryPosition = glm::vec3(0.0f, 0.0f, -100.0f);
    float m_militaryScale = 8.5f;

    // WALKING model transform controls
    glm::vec3 m_walkingPosition = glm::vec3(50.0f, 0.0f, -50.0f);
    float m_walkingScale = 5.0f;

    // Animation clock
    float m_animElapsed = 0.0f;
    bool m_militaryAnimEnabled = true;
    float m_militaryAnimSpeed = 1.0f;
    bool m_walkingAnimEnabled = true;
    float m_walkingAnimSpeed = 1.0f;
};
