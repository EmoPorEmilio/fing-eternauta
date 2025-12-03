#pragma once

#include "BaseScene.h"
#include "Shader.h"
#include "Texture.h"

/**
 * @brief Empty scene - clean 3D world with a test cube
 *
 * This scene provides:
 * - Snow-textured floor plane (from BaseScene)
 * - Debug visualization (grid, axes, gizmo)
 * - A textured cube at the origin for testing
 *
 * Use this as a starting point to build new scenes iteratively.
 */
class EmptyScene : public BaseScene
{
public:
    EmptyScene() = default;
    ~EmptyScene() override;

    bool initialize() override;
    void render(const glm::mat4& view, const glm::mat4& projection,
               const glm::vec3& cameraPos, const glm::vec3& cameraFront,
               LightManager& lightManager) override;
    void cleanup() override;

private:
    void setupCubeGeometry();

    // Cube rendering
    GLuint m_cubeVAO = 0;
    GLuint m_cubeVBO = 0;
    Shader m_cubeShader;
    Texture m_cubeTexture;

    // Cube transform
    glm::vec3 m_cubePosition = glm::vec3(0.0f, 1.0f, 0.0f);  // Centered, sitting on floor
    float m_cubeSize = 2.0f;
};
