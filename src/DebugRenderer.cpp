#include "DebugRenderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

DebugRenderer::DebugRenderer()
{
}

DebugRenderer::~DebugRenderer()
{
    cleanup();
}

bool DebugRenderer::initialize()
{
    // Load shaders
    if (!m_gridShader.loadFromFiles("debug_grid.vert", "debug_grid.frag")) {
        std::cerr << "DebugRenderer: Failed to load grid shaders" << std::endl;
        return false;
    }

    if (!m_lineShader.loadFromFiles("debug_line.vert", "debug_line.frag")) {
        std::cerr << "DebugRenderer: Failed to load line shaders" << std::endl;
        return false;
    }

    setupGridGeometry();
    setupAxesGeometry();
    setupGizmoGeometry();

    std::cout << "DebugRenderer: Initialized successfully" << std::endl;
    return true;
}

void DebugRenderer::cleanup()
{
    if (m_gridVAO != 0) {
        glDeleteVertexArrays(1, &m_gridVAO);
        m_gridVAO = 0;
    }
    if (m_axesVAO != 0) {
        glDeleteVertexArrays(1, &m_axesVAO);
        m_axesVAO = 0;
    }
    if (m_axesVBO != 0) {
        glDeleteBuffers(1, &m_axesVBO);
        m_axesVBO = 0;
    }
    if (m_gizmoVAO != 0) {
        glDeleteVertexArrays(1, &m_gizmoVAO);
        m_gizmoVAO = 0;
    }
    if (m_gizmoVBO != 0) {
        glDeleteBuffers(1, &m_gizmoVBO);
        m_gizmoVBO = 0;
    }
}

void DebugRenderer::render(const glm::mat4& view, const glm::mat4& projection,
                           const glm::vec3& cameraPos, const glm::vec3& cameraFront,
                           int viewportWidth, int viewportHeight)
{
    // Render grid first (with depth test)
    if (m_gridEnabled) {
        renderGrid(view, projection);
    }

    // Render origin axes
    if (m_axesEnabled) {
        renderOriginAxes(view, projection);
    }

    // Render corner gizmo (screen-space, no depth)
    if (m_gizmoEnabled) {
        renderCornerGizmo(view, viewportWidth, viewportHeight);
    }
}

void DebugRenderer::renderGrid(const glm::mat4& view, const glm::mat4& projection)
{
    // Enable blending for transparent grid
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    // Enable depth test but allow writing
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);

    m_gridShader.use();
    m_gridShader.setUniform("uView", view);
    m_gridShader.setUniform("uProj", projection);
    m_gridShader.setUniform("uGridScale", m_gridScale);
    m_gridShader.setUniform("uFadeDistance", m_fadeDistance);
    m_gridShader.setUniform("uAxisColorX", COLOR_AXIS_X);
    m_gridShader.setUniform("uAxisColorZ", COLOR_AXIS_Z);
    m_gridShader.setUniform("uGridColorMinor", COLOR_GRID_MINOR);
    m_gridShader.setUniform("uGridColorMajor", COLOR_GRID_MAJOR);

    glBindVertexArray(m_gridVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    glDepthFunc(GL_LESS);
    glDisable(GL_BLEND);
}

void DebugRenderer::renderOriginAxes(const glm::mat4& view, const glm::mat4& projection)
{
    glEnable(GL_DEPTH_TEST);

    // Increase line width for visibility
    glLineWidth(2.0f);

    m_lineShader.use();
    glm::mat4 mvp = projection * view;
    m_lineShader.setUniform("uMVP", mvp);
    m_lineShader.setUniform("uAlpha", 1.0f);

    glBindVertexArray(m_axesVAO);
    glDrawArrays(GL_LINES, 0, 6); // 3 axes * 2 vertices each
    glBindVertexArray(0);

    glLineWidth(1.0f);
}

void DebugRenderer::renderCornerGizmo(const glm::mat4& view, int viewportWidth, int viewportHeight)
{
    // Disable depth test for screen-space overlay
    glDisable(GL_DEPTH_TEST);

    // Increase line width for gizmo
    glLineWidth(3.0f);

    // Create orthographic projection for screen-space rendering
    // Position gizmo in bottom-left corner
    float gizmoSize = 60.0f;
    float margin = 50.0f;

    // Calculate center position in screen space
    float centerX = margin + gizmoSize * 0.5f;
    float centerY = margin + gizmoSize * 0.5f;

    // Create view matrix that only has rotation (no translation)
    glm::mat4 rotationOnly = glm::mat4(glm::mat3(view));

    // Scale and position the gizmo
    glm::mat4 gizmoModel = glm::mat4(1.0f);
    gizmoModel = glm::translate(gizmoModel, glm::vec3(centerX, centerY, 0.0f));
    gizmoModel = gizmoModel * rotationOnly;
    gizmoModel = glm::scale(gizmoModel, glm::vec3(gizmoSize * 0.4f));

    // Orthographic projection for screen space
    glm::mat4 ortho = glm::ortho(0.0f, (float)viewportWidth, 0.0f, (float)viewportHeight, -100.0f, 100.0f);

    glm::mat4 mvp = ortho * gizmoModel;

    m_lineShader.use();
    m_lineShader.setUniform("uMVP", mvp);
    m_lineShader.setUniform("uAlpha", 1.0f);

    glBindVertexArray(m_gizmoVAO);
    glDrawArrays(GL_LINES, 0, 6); // 3 axes * 2 vertices each
    glBindVertexArray(0);

    glLineWidth(1.0f);
    glEnable(GL_DEPTH_TEST);
}

void DebugRenderer::setupGridGeometry()
{
    // Grid uses fullscreen quad - vertices are generated in shader
    glGenVertexArrays(1, &m_gridVAO);
}

void DebugRenderer::setupAxesGeometry()
{
    // Origin axes: 3 lines from origin along X, Y, Z
    // Each vertex has position (3) + color (3) = 6 floats
    float axisLength = m_axisLength;

    float axesVertices[] = {
        // X axis (red)
        0.0f, 0.0f, 0.0f,  COLOR_AXIS_X.r, COLOR_AXIS_X.g, COLOR_AXIS_X.b,
        axisLength, 0.0f, 0.0f,  COLOR_AXIS_X.r, COLOR_AXIS_X.g, COLOR_AXIS_X.b,

        // Y axis (green)
        0.0f, 0.0f, 0.0f,  COLOR_AXIS_Y.r, COLOR_AXIS_Y.g, COLOR_AXIS_Y.b,
        0.0f, axisLength, 0.0f,  COLOR_AXIS_Y.r, COLOR_AXIS_Y.g, COLOR_AXIS_Y.b,

        // Z axis (blue) - negative Z in OpenGL convention
        0.0f, 0.0f, 0.0f,  COLOR_AXIS_Z.r, COLOR_AXIS_Z.g, COLOR_AXIS_Z.b,
        0.0f, 0.0f, -axisLength,  COLOR_AXIS_Z.r, COLOR_AXIS_Z.g, COLOR_AXIS_Z.b,
    };

    glGenVertexArrays(1, &m_axesVAO);
    glGenBuffers(1, &m_axesVBO);

    glBindVertexArray(m_axesVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_axesVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(axesVertices), axesVertices, GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

    // Color attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}

void DebugRenderer::setupGizmoGeometry()
{
    // Corner gizmo: same as axes but will be transformed to screen space
    float gizmoLength = 1.0f; // Will be scaled in render

    float gizmoVertices[] = {
        // X axis (red)
        0.0f, 0.0f, 0.0f,  COLOR_AXIS_X.r, COLOR_AXIS_X.g, COLOR_AXIS_X.b,
        gizmoLength, 0.0f, 0.0f,  COLOR_AXIS_X.r, COLOR_AXIS_X.g, COLOR_AXIS_X.b,

        // Y axis (green)
        0.0f, 0.0f, 0.0f,  COLOR_AXIS_Y.r, COLOR_AXIS_Y.g, COLOR_AXIS_Y.b,
        0.0f, gizmoLength, 0.0f,  COLOR_AXIS_Y.r, COLOR_AXIS_Y.g, COLOR_AXIS_Y.b,

        // Z axis (blue)
        0.0f, 0.0f, 0.0f,  COLOR_AXIS_Z.r, COLOR_AXIS_Z.g, COLOR_AXIS_Z.b,
        0.0f, 0.0f, gizmoLength,  COLOR_AXIS_Z.r, COLOR_AXIS_Z.g, COLOR_AXIS_Z.b,
    };

    glGenVertexArrays(1, &m_gizmoVAO);
    glGenBuffers(1, &m_gizmoVBO);

    glBindVertexArray(m_gizmoVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_gizmoVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(gizmoVertices), gizmoVertices, GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)0);

    // Color attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void*)(3 * sizeof(float)));

    glBindVertexArray(0);
}
