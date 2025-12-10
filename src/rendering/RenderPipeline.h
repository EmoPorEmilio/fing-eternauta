#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../ecs/Registry.h"
#include "../ecs/systems/RenderSystem.h"
#include "../Shader.h"
#include "../DebugRenderer.h"
#include "../core/GameConfig.h"
#include <vector>

// Consolidates shadow mapping and scene rendering into reusable passes
class RenderPipeline {
public:
    struct ShadowConfig {
        GLuint fbo = 0;
        GLuint depthTexture = 0;
        int width = GameConfig::SHADOW_MAP_SIZE;
        int height = GameConfig::SHADOW_MAP_SIZE;
        Shader* depthShader = nullptr;
    };

    struct RenderConfig {
        Registry* registry = nullptr;
        RenderSystem* renderSystem = nullptr;
        Shader* groundShader = nullptr;
        Shader* colorShader = nullptr;
        Shader* overlayShader = nullptr;
        DebugRenderer* axes = nullptr;
        GLuint planeVAO = 0;
        GLuint snowTexture = 0;
        GLuint overlayVAO = 0;
        float aspectRatio = 16.0f / 9.0f;
        glm::vec3 lightDir = glm::normalize(GameConfig::LIGHT_DIR);
    };

    void setShadowConfig(const ShadowConfig& config) { m_shadowConfig = config; }
    void setRenderConfig(const RenderConfig& config) { m_renderConfig = config; }

    // Execute shadow pass for a set of entities
    glm::mat4 renderShadowPass(const glm::vec3& focusPoint,
                               const std::vector<Entity>& shadowCasters) {
        float orthoSize = GameConfig::SHADOW_ORTHO_SIZE;
        glm::vec3 lightPos = focusPoint + m_renderConfig.lightDir * GameConfig::SHADOW_DISTANCE;
        glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize,
                                                GameConfig::SHADOW_NEAR, GameConfig::SHADOW_FAR);
        glm::mat4 lightView = glm::lookAt(lightPos, focusPoint, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        glViewport(0, 0, m_shadowConfig.width, m_shadowConfig.height);
        glBindFramebuffer(GL_FRAMEBUFFER, m_shadowConfig.fbo);
        glClear(GL_DEPTH_BUFFER_BIT);

        m_shadowConfig.depthShader->use();
        m_shadowConfig.depthShader->setMat4("uLightSpaceMatrix", lightSpaceMatrix);

        for (Entity e : shadowCasters) {
            auto* t = m_renderConfig.registry->getTransform(e);
            if (t && t->position.y > -100.0f) {
                m_shadowConfig.depthShader->setMat4("uModel", t->matrix());
                auto* mg = m_renderConfig.registry->getMeshGroup(e);
                if (mg) {
                    for (const auto& mesh : mg->meshes) {
                        glBindVertexArray(mesh.vao);
                        glDrawElements(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr);
                    }
                }
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);

        return lightSpaceMatrix;
    }

    // Render the 3D scene with shadows
    void renderScene(const glm::mat4& view, const glm::mat4& projection,
                     const glm::mat4& lightSpaceMatrix,
                     const glm::vec3& viewPos,
                     bool fogEnabled, bool shadowsEnabled = true) {

        // Setup render system
        m_renderConfig.renderSystem->setFogEnabled(fogEnabled);
        m_renderConfig.renderSystem->setShadowsEnabled(shadowsEnabled);
        m_renderConfig.renderSystem->setShadowMap(m_shadowConfig.depthTexture);
        m_renderConfig.renderSystem->setLightSpaceMatrix(lightSpaceMatrix);
        m_renderConfig.renderSystem->updateWithView(*m_renderConfig.registry,
                                                     m_renderConfig.aspectRatio, view);

        // Render ground plane
        m_renderConfig.groundShader->use();
        m_renderConfig.groundShader->setMat4("uView", view);
        m_renderConfig.groundShader->setMat4("uProjection", projection);
        m_renderConfig.groundShader->setMat4("uModel", glm::mat4(1.0f));
        m_renderConfig.groundShader->setMat4("uLightSpaceMatrix", lightSpaceMatrix);
        m_renderConfig.groundShader->setVec3("uLightDir", m_renderConfig.lightDir);
        m_renderConfig.groundShader->setVec3("uViewPos", viewPos);
        m_renderConfig.groundShader->setInt("uHasTexture", 1);
        m_renderConfig.groundShader->setInt("uFogEnabled", fogEnabled ? 1 : 0);
        m_renderConfig.groundShader->setInt("uShadowsEnabled", shadowsEnabled ? 1 : 0);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, m_renderConfig.snowTexture);
        m_renderConfig.groundShader->setInt("uTexture", 0);
        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, m_shadowConfig.depthTexture);
        m_renderConfig.groundShader->setInt("uShadowMap", 1);

        glBindVertexArray(m_renderConfig.planeVAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
        glBindVertexArray(0);
    }

    // Render debug axes
    void renderAxes(const glm::mat4& viewProjection) {
        m_renderConfig.colorShader->use();
        m_renderConfig.colorShader->setMat4("uMVP", viewProjection);
        m_renderConfig.axes->draw();
    }

    // Render snow overlay
    void renderSnowOverlay(float gameTime, float snowSpeed, float snowAngle, float snowBlur) {
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        m_renderConfig.overlayShader->use();
        m_renderConfig.overlayShader->setVec3("iResolution",
            glm::vec3((float)GameConfig::WINDOW_WIDTH, (float)GameConfig::WINDOW_HEIGHT, 1.0f));
        m_renderConfig.overlayShader->setFloat("iTime", gameTime);
        m_renderConfig.overlayShader->setFloat("uSnowSpeed", snowSpeed);
        m_renderConfig.overlayShader->setFloat("uSnowDirectionDeg", snowAngle);
        m_renderConfig.overlayShader->setFloat("uMotionBlur", snowBlur);

        glBindVertexArray(m_renderConfig.overlayVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }

private:
    ShadowConfig m_shadowConfig;
    RenderConfig m_renderConfig;
};
