#pragma once

#include <glm/glm.hpp>

class LightManager;

/**
 * @brief Interface for all scenes in the engine
 *
 * Defines the lifecycle methods that all scenes must implement.
 * Scenes are responsible for managing their own content (objects, models, etc.)
 * while the Application handles camera, input, and rendering orchestration.
 */
class IScene
{
public:
    virtual ~IScene() = default;

    /**
     * @brief Initialize scene resources
     * @return true if initialization succeeded
     */
    virtual bool initialize() = 0;

    /**
     * @brief Update scene state
     * @param cameraPos Current camera position
     * @param deltaTime Time since last frame
     * @param viewMatrix Current view matrix
     * @param projectionMatrix Current projection matrix
     */
    virtual void update(const glm::vec3& cameraPos, float deltaTime,
                       const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix) = 0;

    /**
     * @brief Render scene content
     * @param view View matrix
     * @param projection Projection matrix
     * @param cameraPos Camera position
     * @param cameraFront Camera forward direction
     * @param lightManager Reference to light manager for flashlight etc.
     */
    virtual void render(const glm::mat4& view, const glm::mat4& projection,
                       const glm::vec3& cameraPos, const glm::vec3& cameraFront,
                       LightManager& lightManager) = 0;

    /**
     * @brief Clean up scene resources
     */
    virtual void cleanup() = 0;

    // Fog controls (all scenes should support fog)
    virtual void setFogEnabled(bool enabled) = 0;
    virtual void setFogColor(const glm::vec3& color) = 0;
    virtual void setFogDensity(float density) = 0;
    virtual void setFogDesaturationStrength(float strength) = 0;
    virtual void setFogAbsorption(float density, float strength) = 0;

    // Material controls
    virtual void setAmbient(float ambient) = 0;
    virtual void setSpecularStrength(float strength) = 0;
    virtual void setNormalStrength(float strength) = 0;
    virtual void setRoughnessBias(float bias) = 0;

    // Debug visualization controls
    virtual void setDebugGridEnabled(bool enabled) = 0;
    virtual void setDebugAxesEnabled(bool enabled) = 0;
    virtual void setDebugGizmoEnabled(bool enabled) = 0;
    virtual void setDebugGridScale(float scale) = 0;
    virtual void setDebugGridFadeDistance(float distance) = 0;
    virtual void setViewportSize(int width, int height) = 0;

    // Floor mode
    enum class FloorMode { GridOnly, TexturedSnow, Both };
    virtual void setFloorMode(FloorMode mode) = 0;
};
