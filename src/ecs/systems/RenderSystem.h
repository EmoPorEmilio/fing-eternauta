#pragma once
#include "../Registry.h"
#include "../../Shader.h"
#include <glad/glad.h>

class RenderSystem {
public:
    void loadShaders() {
        m_colorShader.loadFromFiles("shaders/color.vert", "shaders/color.frag");
        m_modelShader.loadFromFiles("shaders/model.vert", "shaders/model.frag");
        m_skinnedShader.loadFromFiles("shaders/skinned.vert", "shaders/model.frag");
    }

    void update(Registry& registry, float aspectRatio) {
        Entity camEntity = registry.getActiveCamera();
        if (camEntity == NULL_ENTITY) return;

        auto* cam = registry.getCamera(camEntity);
        auto* camTransform = registry.getTransform(camEntity);
        if (!cam || !camTransform) return;

        // Compute view matrix: look ahead of character
        glm::mat4 view;
        auto* followTarget = registry.getFollowTarget(camEntity);
        if (followTarget && followTarget->target != NULL_ENTITY) {
            auto* targetTransform = registry.getTransform(followTarget->target);
            auto* facing = registry.getFacingDirection(followTarget->target);
            if (targetTransform && facing) {
                // Look at point ahead of character based on target's facing yaw
                float yawRad = glm::radians(facing->yaw);
                glm::vec3 forward(-sin(yawRad), 0.0f, -cos(yawRad));
                glm::vec3 lookAtPos = targetTransform->position + forward * followTarget->lookAhead;
                lookAtPos.y += 1.0f; // Eye level
                view = glm::lookAt(camTransform->position, lookAtPos, glm::vec3(0.0f, 1.0f, 0.0f));
            } else {
                view = glm::lookAt(camTransform->position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            }
        } else {
            view = glm::lookAt(camTransform->position, glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
        }

        glm::mat4 projection = cam->projectionMatrix(aspectRatio);
        glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));

        registry.forEachRenderable([&](Entity entity, Transform& transform, MeshGroup& meshGroup, Renderable& renderable) {
            Shader* shader = getShader(renderable.shader);
            if (!shader) return;

            shader->use();
            shader->setMat4("uView", view);
            shader->setMat4("uProjection", projection);
            shader->setMat4("uModel", transform.matrix());

            bool hasTexture = false;
            for (const auto& mesh : meshGroup.meshes) {
                if (mesh.texture) { hasTexture = true; break; }
            }

            if (renderable.shader == ShaderType::Model || renderable.shader == ShaderType::Skinned) {
                shader->setVec3("uLightDir", lightDir);
                shader->setVec3("uViewPos", camTransform->position);
                shader->setInt("uTexture", 0);
                shader->setInt("uHasTexture", hasTexture ? 1 : 0);
            }

            if (renderable.shader == ShaderType::Skinned) {
                auto* skeleton = registry.getSkeleton(entity);
                shader->setInt("uUseSkinning", (skeleton && !skeleton->boneMatrices.empty()) ? 1 : 0);
                if (skeleton && !skeleton->boneMatrices.empty()) {
                    shader->setMat4Array("uBones", skeleton->boneMatrices);
                }
            }

            for (const auto& mesh : meshGroup.meshes) {
                if (mesh.texture) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mesh.texture);
                }

                glBindVertexArray(mesh.vao);
                glDrawElements(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr);
            }
        });

        glBindVertexArray(0);
    }

    // Render with a custom view matrix (for god mode)
    void updateWithView(Registry& registry, float aspectRatio, const glm::mat4& view) {
        Entity camEntity = registry.getActiveCamera();
        if (camEntity == NULL_ENTITY) return;

        auto* cam = registry.getCamera(camEntity);
        auto* camTransform = registry.getTransform(camEntity);
        if (!cam || !camTransform) return;

        glm::mat4 projection = cam->projectionMatrix(aspectRatio);
        glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));

        registry.forEachRenderable([&](Entity entity, Transform& transform, MeshGroup& meshGroup, Renderable& renderable) {
            Shader* shader = getShader(renderable.shader);
            if (!shader) return;

            shader->use();
            shader->setMat4("uView", view);
            shader->setMat4("uProjection", projection);
            shader->setMat4("uModel", transform.matrix());

            bool hasTexture = false;
            for (const auto& mesh : meshGroup.meshes) {
                if (mesh.texture) { hasTexture = true; break; }
            }

            if (renderable.shader == ShaderType::Model || renderable.shader == ShaderType::Skinned) {
                shader->setVec3("uLightDir", lightDir);
                shader->setVec3("uViewPos", camTransform->position);
                shader->setInt("uTexture", 0);
                shader->setInt("uHasTexture", hasTexture ? 1 : 0);
            }

            if (renderable.shader == ShaderType::Skinned) {
                auto* skeleton = registry.getSkeleton(entity);
                shader->setInt("uUseSkinning", (skeleton && !skeleton->boneMatrices.empty()) ? 1 : 0);
                if (skeleton && !skeleton->boneMatrices.empty()) {
                    shader->setMat4Array("uBones", skeleton->boneMatrices);
                }
            }

            for (const auto& mesh : meshGroup.meshes) {
                if (mesh.texture) {
                    glActiveTexture(GL_TEXTURE0);
                    glBindTexture(GL_TEXTURE_2D, mesh.texture);
                }

                glBindVertexArray(mesh.vao);
                glDrawElements(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr);
            }
        });

        glBindVertexArray(0);
    }

private:
    Shader m_colorShader;
    Shader m_modelShader;
    Shader m_skinnedShader;

    Shader* getShader(ShaderType type) {
        switch (type) {
            case ShaderType::Color: return &m_colorShader;
            case ShaderType::Model: return &m_modelShader;
            case ShaderType::Skinned: return &m_skinnedShader;
        }
        return nullptr;
    }
};
