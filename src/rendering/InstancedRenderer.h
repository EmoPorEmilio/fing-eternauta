#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include "../Shader.h"
#include "../ecs/components/Mesh.h"

// Instance data for a single building
struct BuildingInstance {
    glm::mat4 modelMatrix;  // Full model transformation matrix
};

// Handles instanced rendering of buildings
// Instead of N draw calls for N buildings, uses 1 draw call with instance data
class InstancedRenderer {
public:
    InstancedRenderer() = default;
    ~InstancedRenderer() {
        cleanup();
    }

    InstancedRenderer(const InstancedRenderer&) = delete;
    InstancedRenderer& operator=(const InstancedRenderer&) = delete;

    void init(size_t maxInstances) {
        m_maxInstances = maxInstances;
        m_instances.reserve(maxInstances);

        // Create instance buffer (stores model matrices)
        glCreateBuffers(1, &m_instanceBuffer);
        glNamedBufferStorage(m_instanceBuffer,
                             maxInstances * sizeof(BuildingInstance),
                             nullptr,
                             GL_DYNAMIC_STORAGE_BIT);
    }

    void cleanup() {
        if (m_instanceBuffer) {
            glDeleteBuffers(1, &m_instanceBuffer);
            m_instanceBuffer = 0;
        }
    }

    // Clear instances for new frame
    void beginFrame() {
        m_instances.clear();
    }

    // Add a building instance
    void addInstance(const glm::vec3& position, const glm::vec3& scale) {
        if (m_instances.size() >= m_maxInstances) return;

        BuildingInstance instance;
        instance.modelMatrix = glm::translate(glm::mat4(1.0f), position);
        instance.modelMatrix = glm::scale(instance.modelMatrix, scale);
        m_instances.push_back(instance);
    }

    // Upload instance data to GPU and render
    void render(const Mesh& mesh, Shader& shader) {
        if (m_instances.empty()) return;

        // Upload instance data
        glNamedBufferSubData(m_instanceBuffer, 0,
                             m_instances.size() * sizeof(BuildingInstance),
                             m_instances.data());

        // Bind mesh VAO
        glBindVertexArray(mesh.vao);

        // Setup instanced attribute (mat4 requires 4 vec4 attribute slots)
        glBindBuffer(GL_ARRAY_BUFFER, m_instanceBuffer);

        // Model matrix takes attribute locations 5, 6, 7, 8 (4 vec4s)
        for (int i = 0; i < 4; ++i) {
            GLuint attribLoc = 5 + i;
            glEnableVertexAttribArray(attribLoc);
            glVertexAttribPointer(attribLoc,
                                  4,                                    // vec4
                                  GL_FLOAT,
                                  GL_FALSE,
                                  sizeof(BuildingInstance),             // stride
                                  (void*)(sizeof(glm::vec4) * i));      // offset into mat4
            glVertexAttribDivisor(attribLoc, 1);  // One matrix per instance
        }

        // Draw all instances
        shader.use();
        glDrawElementsInstanced(GL_TRIANGLES, mesh.indexCount, mesh.indexType,
                                nullptr, static_cast<GLsizei>(m_instances.size()));

        // Cleanup instanced attributes
        for (int i = 0; i < 4; ++i) {
            glVertexAttribDivisor(5 + i, 0);
            glDisableVertexAttribArray(5 + i);
        }

        glBindVertexArray(0);
    }

    // Render shadow pass (depth only)
    void renderShadow(const Mesh& mesh, Shader& depthShader, const glm::mat4& lightSpaceMatrix) {
        if (m_instances.empty()) return;

        // Upload instance data
        glNamedBufferSubData(m_instanceBuffer, 0,
                             m_instances.size() * sizeof(BuildingInstance),
                             m_instances.data());

        glBindVertexArray(mesh.vao);

        // Setup instanced attributes
        glBindBuffer(GL_ARRAY_BUFFER, m_instanceBuffer);
        for (int i = 0; i < 4; ++i) {
            GLuint attribLoc = 5 + i;
            glEnableVertexAttribArray(attribLoc);
            glVertexAttribPointer(attribLoc, 4, GL_FLOAT, GL_FALSE,
                                  sizeof(BuildingInstance),
                                  (void*)(sizeof(glm::vec4) * i));
            glVertexAttribDivisor(attribLoc, 1);
        }

        depthShader.use();
        depthShader.setMat4("uLightSpaceMatrix", lightSpaceMatrix);

        glDrawElementsInstanced(GL_TRIANGLES, mesh.indexCount, mesh.indexType,
                                nullptr, static_cast<GLsizei>(m_instances.size()));

        for (int i = 0; i < 4; ++i) {
            glVertexAttribDivisor(5 + i, 0);
            glDisableVertexAttribArray(5 + i);
        }

        glBindVertexArray(0);
    }

    size_t getInstanceCount() const { return m_instances.size(); }

private:
    GLuint m_instanceBuffer = 0;
    size_t m_maxInstances = 0;
    std::vector<BuildingInstance> m_instances;
};
