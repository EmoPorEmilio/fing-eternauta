#include "EmptyScene.h"
#include "LightManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

EmptyScene::~EmptyScene()
{
    cleanup();
}

bool EmptyScene::initialize()
{
    // Initialize base scene (floor, debug renderer)
    if (!BaseScene::initialize()) {
        return false;
    }

    // Setup cube
    setupCubeGeometry();

    // Load cube shader (reuse phong_notess)
    if (!m_cubeShader.loadFromFiles("phong_notess.vert", "phong_notess.frag")) {
        std::cerr << "EmptyScene: Failed to load cube shader" << std::endl;
        return false;
    }

    // Load snow texture for the cube
    if (!m_cubeTexture.loadFromFile("snow/snow_02_diff_1k.jpg", true, true)) {
        std::cerr << "EmptyScene: Failed to load cube texture" << std::endl;
        return false;
    }

    std::cout << "EmptyScene: Initialized with test cube at origin" << std::endl;
    return true;
}

void EmptyScene::render(const glm::mat4& view, const glm::mat4& projection,
                        const glm::vec3& cameraPos, const glm::vec3& cameraFront,
                        LightManager& lightManager)
{
    // Render base scene (floor, debug visualization)
    BaseScene::render(view, projection, cameraPos, cameraFront, lightManager);

    // Render cube
    m_cubeShader.use();

    // Model matrix - position the cube
    glm::mat4 model = glm::mat4(1.0f);
    model = glm::translate(model, m_cubePosition);
    model = glm::scale(model, glm::vec3(m_cubeSize));

    m_cubeShader.setUniform("uModel", model);
    m_cubeShader.setUniform("uView", view);
    m_cubeShader.setUniform("uProj", projection);
    m_cubeShader.setUniform("uViewPos", cameraPos);
    m_cubeShader.setUniform("uLightPos", glm::vec3(5.0f, 10.0f, 5.0f));
    m_cubeShader.setUniform("uLightColor", glm::vec3(1.0f));
    m_cubeShader.setUniform("uObjectColor", glm::vec3(1.0f));
    m_cubeShader.setUniform("uAmbient", 0.3f);
    m_cubeShader.setUniform("uSpecularStrength", 0.2f);
    m_cubeShader.setUniform("uCullDistance", 1000.0f);

    // Flashlight
    m_cubeShader.setUniform("uFlashlightOn", lightManager.isFlashlightOn());
    m_cubeShader.setUniform("uFlashlightPos", cameraPos);
    m_cubeShader.setUniform("uFlashlightDir", cameraFront);
    m_cubeShader.setUniform("uFlashlightCutoff", lightManager.getFlashlightCutoff());
    m_cubeShader.setUniform("uFlashlightBrightness", lightManager.getFlashlightBrightness());
    m_cubeShader.setUniform("uFlashlightColor", lightManager.getFlashlightColor());

    // Fog
    m_cubeShader.setUniform("uFogEnabled", m_fogEnabled);
    m_cubeShader.setUniform("uFogColor", m_fogColor);
    m_cubeShader.setUniform("uFogDensity", m_fogDensity);
    m_cubeShader.setUniform("uFogDesaturationStrength", m_fogDesaturationStrength);
    m_cubeShader.setUniform("uFogAbsorptionDensity", m_fogAbsorptionDensity);
    m_cubeShader.setUniform("uFogAbsorptionStrength", m_fogAbsorptionStrength);
    m_cubeShader.setUniform("uBackgroundColor", glm::vec3(0.157f));

    // Texture
    m_cubeTexture.bind(0);
    m_cubeShader.setUniform("uAlbedoTex", 0);
    m_cubeShader.setUniform("uNormalStrength", 0.0f);  // No normal map for cube
    m_cubeShader.setUniform("uRoughnessBias", 0.0f);

    glBindVertexArray(m_cubeVAO);
    glDrawArrays(GL_TRIANGLES, 0, 36);
    glBindVertexArray(0);
}

void EmptyScene::cleanup()
{
    if (m_cubeVAO != 0) {
        glDeleteVertexArrays(1, &m_cubeVAO);
        m_cubeVAO = 0;
    }
    if (m_cubeVBO != 0) {
        glDeleteBuffers(1, &m_cubeVBO);
        m_cubeVBO = 0;
    }
    m_cubeShader.cleanup();
    // Texture cleanup handled by destructor

    BaseScene::cleanup();
}

void EmptyScene::setupCubeGeometry()
{
    // Cube vertices: position (3) + normal (3) + uv (2) = 8 floats per vertex
    // 6 faces * 2 triangles * 3 vertices = 36 vertices
    float vertices[] = {
        // Back face (Z-)
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  1.0f, 1.0f,
        -0.5f, -0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  0.0f, -1.0f,  0.0f, 1.0f,

        // Front face (Z+)
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  1.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 1.0f,
        -0.5f, -0.5f,  0.5f,  0.0f,  0.0f,  1.0f,  0.0f, 0.0f,

        // Left face (X-)
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
        -0.5f,  0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f, -1.0f,  0.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f,  0.5f, -1.0f,  0.0f,  0.0f,  1.0f, 1.0f,

        // Right face (X+)
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  1.0f,  0.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  1.0f,  0.0f,  0.0f,  0.0f, 0.0f,

        // Bottom face (Y-)
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,
         0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 1.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
         0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f, -0.5f,  0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 0.0f,
        -0.5f, -0.5f, -0.5f,  0.0f, -1.0f,  0.0f,  0.0f, 1.0f,

        // Top face (Y+)
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
         0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 1.0f,
         0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  1.0f, 0.0f,
        -0.5f,  0.5f, -0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 1.0f,
        -0.5f,  0.5f,  0.5f,  0.0f,  1.0f,  0.0f,  0.0f, 0.0f,
    };

    glGenVertexArrays(1, &m_cubeVAO);
    glGenBuffers(1, &m_cubeVBO);

    glBindVertexArray(m_cubeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_cubeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    // UV
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
}
