#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>

// Uniform Buffer Object for shared data across shaders
// Reduces redundant uniform updates per frame

// Layout matches std140 specification for GPU memory alignment
// Binding point 0: CameraData (view, projection, viewPos)
// Binding point 1: LightData (lightDir, lightSpaceMatrix)

struct CameraUBO {
    glm::mat4 view;           // offset 0
    glm::mat4 projection;     // offset 64
    glm::vec4 viewPos;        // offset 128 (vec3 padded to vec4 for std140)
    // Total: 144 bytes
};

struct LightUBO {
    glm::mat4 lightSpaceMatrix;  // offset 0
    glm::vec4 lightDir;          // offset 64 (vec3 padded to vec4 for std140)
    // Total: 80 bytes
};

class UniformBuffer {
public:
    UniformBuffer() = default;
    ~UniformBuffer() {
        if (m_cameraUBO) glDeleteBuffers(1, &m_cameraUBO);
        if (m_lightUBO) glDeleteBuffers(1, &m_lightUBO);
    }

    UniformBuffer(const UniformBuffer&) = delete;
    UniformBuffer& operator=(const UniformBuffer&) = delete;

    void init() {
        // Create Camera UBO at binding point 0
        glCreateBuffers(1, &m_cameraUBO);
        glNamedBufferStorage(m_cameraUBO, sizeof(CameraUBO), nullptr, GL_DYNAMIC_STORAGE_BIT);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, m_cameraUBO);

        // Create Light UBO at binding point 1
        glCreateBuffers(1, &m_lightUBO);
        glNamedBufferStorage(m_lightUBO, sizeof(LightUBO), nullptr, GL_DYNAMIC_STORAGE_BIT);
        glBindBufferBase(GL_UNIFORM_BUFFER, 1, m_lightUBO);
    }

    void updateCamera(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& viewPos) {
        CameraUBO data;
        data.view = view;
        data.projection = projection;
        data.viewPos = glm::vec4(viewPos, 1.0f);  // Pad to vec4
        glNamedBufferSubData(m_cameraUBO, 0, sizeof(CameraUBO), &data);
    }

    void updateLight(const glm::mat4& lightSpaceMatrix, const glm::vec3& lightDir) {
        LightUBO data;
        data.lightSpaceMatrix = lightSpaceMatrix;
        data.lightDir = glm::vec4(lightDir, 0.0f);  // Pad to vec4
        glNamedBufferSubData(m_lightUBO, 0, sizeof(LightUBO), &data);
    }

    // Bind a shader's uniform block to the UBO binding points
    static void bindShaderUBOs(GLuint program) {
        GLuint cameraIndex = glGetUniformBlockIndex(program, "CameraData");
        if (cameraIndex != GL_INVALID_INDEX) {
            glUniformBlockBinding(program, cameraIndex, 0);
        }

        GLuint lightIndex = glGetUniformBlockIndex(program, "LightData");
        if (lightIndex != GL_INVALID_INDEX) {
            glUniformBlockBinding(program, lightIndex, 1);
        }
    }

private:
    GLuint m_cameraUBO = 0;
    GLuint m_lightUBO = 0;
};
