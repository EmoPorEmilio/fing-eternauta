#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Shader.h"

/**
 * @brief Blender-style debug visualization renderer
 *
 * Provides:
 * - Infinite grid with distance fade (Y=0 plane)
 * - Origin axes (RGB = XYZ)
 * - Corner orientation gizmo
 * - Configurable colors and scale
 */
class DebugRenderer
{
public:
    DebugRenderer();
    ~DebugRenderer();

    /**
     * @brief Initialize debug rendering resources
     * @return true if initialization succeeded
     */
    bool initialize();

    /**
     * @brief Render debug elements
     * @param view View matrix
     * @param projection Projection matrix
     * @param cameraPos Camera world position
     * @param cameraFront Camera forward direction
     * @param viewportWidth Viewport width for gizmo positioning
     * @param viewportHeight Viewport height for gizmo positioning
     */
    void render(const glm::mat4& view, const glm::mat4& projection,
                const glm::vec3& cameraPos, const glm::vec3& cameraFront,
                int viewportWidth, int viewportHeight);

    /**
     * @brief Clean up GPU resources
     */
    void cleanup();

    // Toggle controls
    void setGridEnabled(bool enabled) { m_gridEnabled = enabled; }
    void setAxesEnabled(bool enabled) { m_axesEnabled = enabled; }
    void setGizmoEnabled(bool enabled) { m_gizmoEnabled = enabled; }

    bool isGridEnabled() const { return m_gridEnabled; }
    bool isAxesEnabled() const { return m_axesEnabled; }
    bool isGizmoEnabled() const { return m_gizmoEnabled; }

    // Grid configuration
    void setGridScale(float scale) { m_gridScale = scale; }
    void setGridFadeDistance(float dist) { m_fadeDistance = dist; }

    float getGridScale() const { return m_gridScale; }
    float getGridFadeDistance() const { return m_fadeDistance; }

    // Blender theme colors
    static constexpr glm::vec3 COLOR_AXIS_X = glm::vec3(0.902f, 0.224f, 0.275f);  // #E63946
    static constexpr glm::vec3 COLOR_AXIS_Y = glm::vec3(0.322f, 0.718f, 0.533f);  // #52B788
    static constexpr glm::vec3 COLOR_AXIS_Z = glm::vec3(0.282f, 0.584f, 0.937f);  // #4895EF
    static constexpr glm::vec3 COLOR_GRID_MINOR = glm::vec3(0.239f, 0.239f, 0.239f);  // #3D3D3D
    static constexpr glm::vec3 COLOR_GRID_MAJOR = glm::vec3(0.353f, 0.353f, 0.353f);  // #5A5A5A
    static constexpr glm::vec3 COLOR_BACKGROUND = glm::vec3(0.157f, 0.157f, 0.157f);  // #282828

private:
    void renderGrid(const glm::mat4& view, const glm::mat4& projection);
    void renderOriginAxes(const glm::mat4& view, const glm::mat4& projection);
    void renderCornerGizmo(const glm::mat4& view, int viewportWidth, int viewportHeight);

    void setupGridGeometry();
    void setupAxesGeometry();
    void setupGizmoGeometry();

    // Grid rendering
    Shader m_gridShader;
    GLuint m_gridVAO = 0;

    // Axes rendering (world-space origin axes)
    Shader m_lineShader;
    GLuint m_axesVAO = 0;
    GLuint m_axesVBO = 0;

    // Corner gizmo (screen-space orientation indicator)
    GLuint m_gizmoVAO = 0;
    GLuint m_gizmoVBO = 0;

    // State
    bool m_gridEnabled = true;
    bool m_axesEnabled = true;
    bool m_gizmoEnabled = true;

    // Configuration
    float m_gridScale = 1.0f;      // 1.0 = 1 meter grid cells
    float m_fadeDistance = 150.0f; // Grid fade distance
    float m_axisLength = 1.0f;     // Origin axis length
};
