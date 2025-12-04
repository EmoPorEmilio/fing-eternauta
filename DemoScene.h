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

    // Walking model controls (now controlled via event system)
    void setWalkingPosition(const glm::vec3& p) { m_walkingPosition = p; }
    void setWalkingScale(float s) { m_walkingScale = s; }
    void setWalkingAnimEnabled(bool e) { m_walkingAnimEnabled = e; }
    void setWalkingAnimSpeed(float s) { m_walkingAnimSpeed = s; }

    // Monster-2 model controls (now controlled via event system)
    void setMonster2Position(const glm::vec3& p) { m_monster2Position = p; }
    void setMonster2Scale(float s) { m_monster2Scale = s; }
    void setMonster2AnimEnabled(bool e) { m_monster2AnimEnabled = e; }
    void setMonster2AnimSpeed(float s) { m_monster2AnimSpeed = s; }

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
    int m_monster2InstanceId = -1;

    // Snow system
    SnowSystem m_snowSystem;

    // Rendering parameters
    float m_cullDistance = 200.0f;

    // FING model transform controls (not currently used)
    glm::vec3 m_fingPosition = glm::vec3(0.0f, 119.900f, -222.300f);
    float m_fingScale = 21.3f;

    // WALKING model config (controlled by UI)
    bool m_walkingEnabled = true;
    glm::vec3 m_walkingPosition = glm::vec3(-3.0f, 0.0f, -5.0f);
    float m_walkingScale = 1000.0f;
    bool m_walkingAnimEnabled = true;
    float m_walkingAnimSpeed = 1.0f;

    // MONSTER-2 model config (controlled by UI)
    bool m_monster2Enabled = true;
    glm::vec3 m_monster2Position = glm::vec3(3.0f, 0.0f, -5.0f);
    float m_monster2Scale = 1000.0f;
    bool m_monster2AnimEnabled = true;
    float m_monster2AnimSpeed = 1.0f;

    // Event subscription
    void onModelsConfigChanged(const events::ModelsConfigChangedEvent& event);
    events::SubscriptionId m_modelsSubscription = 0;
};
