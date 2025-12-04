#pragma once

#include "IScene.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"
#include "Texture.h"
#include "DebugRenderer.h"
#include "events/Events.h"

/**
 * @brief Base scene with 3D world infrastructure
 *
 * Provides the foundational 3D world setup:
 * - Floor plane with snow texture and normal mapping
 * - Fog system (exponential fog with desaturation)
 * - Debug visualization (Blender-style grid, axes, gizmo)
 * - Basic lighting uniforms
 *
 * Derived scenes can add objects, models, particles, etc.
 */
class BaseScene : public IScene
{
public:
    // Use FloorMode from IScene
    using FloorMode = IScene::FloorMode;

    BaseScene();
    virtual ~BaseScene();

    // IScene interface
    bool initialize() override;
    void update(const glm::vec3& cameraPos, float deltaTime,
               const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) override;
    void render(const glm::mat4& view, const glm::mat4& projection,
               const glm::vec3& cameraPos, const glm::vec3& cameraFront,
               LightManager& lightManager) override;
    void cleanup() override;

    // Fog controls
    void setFogEnabled(bool enabled) override { m_fogEnabled = enabled; }
    void setFogColor(const glm::vec3& color) override { m_fogColor = color; }
    void setFogDensity(float density) override { m_fogDensity = density; }
    void setFogDesaturationStrength(float strength) override { m_fogDesaturationStrength = strength; }
    void setFogAbsorption(float density, float strength) override {
        m_fogAbsorptionDensity = density;
        m_fogAbsorptionStrength = strength;
    }

    // Floor material controls (IScene interface)
    void setAmbient(float v) override { m_ambient = v; }
    void setSpecularStrength(float v) override { m_specularStrength = v; }
    void setNormalStrength(float v) override { m_normalStrength = v; }
    void setRoughnessBias(float v) override { m_roughnessBias = v; }

    // Floor mode control (IScene interface)
    void setFloorMode(FloorMode mode) override { m_floorMode = mode; }
    FloorMode getFloorMode() const { return m_floorMode; }

    // Debug visualization controls (IScene interface)
    void setDebugGridEnabled(bool enabled) override { m_debugRenderer.setGridEnabled(enabled); }
    void setDebugAxesEnabled(bool enabled) override { m_debugRenderer.setAxesEnabled(enabled); }
    void setDebugGizmoEnabled(bool enabled) override { m_debugRenderer.setGizmoEnabled(enabled); }
    void setDebugGridScale(float scale) override { m_debugRenderer.setGridScale(scale); }
    void setDebugGridFadeDistance(float dist) override { m_debugRenderer.setGridFadeDistance(dist); }

    bool isDebugGridEnabled() const { return m_debugRenderer.isGridEnabled(); }
    bool isDebugAxesEnabled() const { return m_debugRenderer.isAxesEnabled(); }
    bool isDebugGizmoEnabled() const { return m_debugRenderer.isGizmoEnabled(); }
    float getDebugGridScale() const { return m_debugRenderer.getGridScale(); }
    float getDebugGridFadeDistance() const { return m_debugRenderer.getGridFadeDistance(); }

    // Viewport dimensions (IScene interface)
    void setViewportSize(int width, int height) override {
        m_viewportWidth = width;
        m_viewportHeight = height;
    }

protected:
    // Floor rendering
    void renderFloor(const glm::mat4& view, const glm::mat4& projection,
                    const glm::vec3& cameraPos, const glm::vec3& cameraFront,
                    LightManager& lightManager);

    // Fog state (accessible to derived classes)
    bool m_fogEnabled = true;
    glm::vec3 m_fogColor = glm::vec3(0.1098f, 0.1255f, 0.1490f);
    float m_fogDensity = 0.0107f;
    float m_fogDesaturationStrength = 0.48f;
    float m_fogAbsorptionDensity = 0.02f;
    float m_fogAbsorptionStrength = 0.8f;

private:
    void setupFloorGeometry();
    void setupFloorShader();

    // Floor plane
    GLuint m_floorVAO = 0;
    GLuint m_floorVBO = 0;
    Shader m_floorShader;
    Texture m_albedoTex;
    Texture m_roughnessTex;
    Texture m_translucencyTex;
    Texture m_heightTex;

    // Floor material parameters
    float m_ambient = 0.2f;
    float m_specularStrength = 0.5f;
    float m_normalStrength = 0.276f;
    float m_roughnessBias = 0.0f;

    // Debug visualization
    DebugRenderer m_debugRenderer;
    FloorMode m_floorMode = FloorMode::GridOnly;

    // Viewport size for gizmo positioning
    int m_viewportWidth = 960;
    int m_viewportHeight = 540;

    // Event handling
    void subscribeToEvents();
    void unsubscribeFromEvents();
    void onFogConfigChanged(const events::FogConfigChangedEvent& event);
    void onMaterialConfigChanged(const events::MaterialConfigChangedEvent& event);
    void onDebugVisualsChanged(const events::DebugVisualsChangedEvent& event);

    events::SubscriptionId m_fogSubscription = 0;
    events::SubscriptionId m_materialSubscription = 0;
    events::SubscriptionId m_debugVisualsSubscription = 0;
};
