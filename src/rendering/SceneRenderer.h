#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../ecs/Registry.h"
#include "../ecs/systems/RenderSystem.h"
#include "../Shader.h"
#include "../DebugRenderer.h"
#include "../core/GameConfig.h"
#include "../core/GameState.h"
#include "../culling/BuildingCuller.h"
#include "../scenes/RenderHelpers.h"

// Encapsulates shared 3D scene rendering used by PlayGame, GodMode, and Cinematic scenes
// This prevents code duplication across scene classes
class SceneRenderer {
public:
    // Configuration set once at init
    struct Config {
        // Shaders
        Shader* groundShader = nullptr;
        Shader* colorShader = nullptr;
        Shader* overlayShader = nullptr;
        Shader* sunShader = nullptr;
        Shader* cometShader = nullptr;
        Shader* depthShader = nullptr;
        Shader* buildingInstancedShader = nullptr;
        Shader* depthInstancedShader = nullptr;

        // Textures
        GLuint snowTexture = 0;
        GLuint brickTexture = 0;
        GLuint brickNormalMap = 0;
        GLuint shadowDepthTexture = 0;

        // VAOs
        GLuint planeVAO = 0;
        GLuint overlayVAO = 0;
        GLuint sunVAO = 0;

        // FBOs
        GLuint shadowFBO = 0;
        GLuint msaaFBO = 0;
        GLuint toonFBO = 0;

        // Geometry
        Mesh* buildingBoxMesh = nullptr;
        MeshGroup* cometMeshGroup = nullptr;

        // Light
        glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));

        // Comet settings
        int numComets = 12;
        float cometFallSpeed = 12.5f;
        float cometCycleTime = 20.0f;
        float cometFallDistance = 500.0f;
        float cometScale = 8.0f;
        glm::vec3 cometFallDir = glm::normalize(glm::vec3(0.4f, -0.6f, 0.4f));
        glm::vec3 cometColor = glm::vec3(1.0f, 0.6f, 0.2f);

        // Debug
        AxisRenderer* axes = nullptr;
    };

    // Per-frame render parameters
    struct FrameParams {
        Registry* registry = nullptr;
        RenderSystem* renderSystem = nullptr;
        BuildingCuller* buildingCuller = nullptr;
        GameState* gameState = nullptr;

        glm::mat4 view;
        glm::mat4 projection;
        glm::vec3 cameraPos;
        float aspectRatio = 16.0f / 9.0f;
        float buildingMaxRenderDistance = 200.0f;

        // Shadow focus point (usually player position)
        glm::vec3 shadowFocusPoint = glm::vec3(0.0f);

        // Entity for FING building shadow
        Entity fingBuilding = NULL_ENTITY;
    };

    void setConfig(const Config& config) { m_config = config; }

    // Render full 3D scene: shadow pass + scene pass
    // Does NOT handle post-processing (toon, motion blur) - that's scene-specific
    // Returns lightSpaceMatrix for use in post-processing
    glm::mat4 renderScene(const FrameParams& params, bool renderToToonFBO = false) {
        // === SHADOW PASS ===
        glm::mat4 lightSpaceMatrix = renderShadowPass(params);

        // === MAIN SCENE PASS ===
        // Choose target FBO
        if (renderToToonFBO) {
            glBindFramebuffer(GL_FRAMEBUFFER, m_config.toonFBO);
        } else {
            glBindFramebuffer(GL_FRAMEBUFFER, m_config.msaaFBO);
        }

        glViewport(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
        glClearColor(0.2f, 0.2f, 0.22f, 1.0f);  // Dark gray sky
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Debug axes
        if (m_config.axes) {
            glm::mat4 vp = params.projection * params.view;
            m_config.colorShader->use();
            m_config.colorShader->setMat4("uMVP", vp);
            m_config.axes->draw();
        }

        // Setup render system with shadows
        RenderHelpers::setupRenderSystem(*params.renderSystem, params.gameState->fogEnabled,
                                          true, m_config.shadowDepthTexture, lightSpaceMatrix);

        // Render ECS entities (protagonist, FING building, etc.)
        params.renderSystem->updateWithView(*params.registry, params.aspectRatio, params.view);

        // Render instanced buildings
        renderBuildings(params, lightSpaceMatrix);

        // Render ground plane
        RenderHelpers::renderGroundPlane(*m_config.groundShader, params.view, params.projection,
            lightSpaceMatrix, m_config.lightDir, params.cameraPos,
            params.gameState->fogEnabled, true, m_config.snowTexture,
            m_config.shadowDepthTexture, m_config.planeVAO);

        // Render sun billboard
        renderSun(params);

        // Render comets
        renderComets(params);

        // Render snow overlay
        RenderHelpers::renderSnowOverlay(*m_config.overlayShader, m_config.overlayVAO, *params.gameState);

        return lightSpaceMatrix;
    }

private:
    Config m_config;

    glm::mat4 renderShadowPass(const FrameParams& params) {
        float orthoSize = GameConfig::SHADOW_ORTHO_SIZE;
        glm::vec3 lightPos = params.shadowFocusPoint + m_config.lightDir * GameConfig::SHADOW_DISTANCE;
        glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize,
                                                GameConfig::SHADOW_NEAR, GameConfig::SHADOW_FAR);
        glm::mat4 lightView = glm::lookAt(lightPos, params.shadowFocusPoint, glm::vec3(0.0f, 1.0f, 0.0f));
        glm::mat4 lightSpaceMatrix = lightProjection * lightView;

        glViewport(0, 0, GameConfig::SHADOW_MAP_SIZE, GameConfig::SHADOW_MAP_SIZE);
        glBindFramebuffer(GL_FRAMEBUFFER, m_config.shadowFBO);
        glClear(GL_DEPTH_BUFFER_BIT);

        // Update and render building shadow casters
        params.buildingCuller->updateShadowCasters(lightSpaceMatrix, params.cameraPos,
                                                    params.buildingMaxRenderDistance);
        params.buildingCuller->renderShadows(*m_config.buildingBoxMesh, *m_config.depthInstancedShader,
                                              lightSpaceMatrix);

        // Render FING building to shadow map
        if (params.fingBuilding != NULL_ENTITY) {
            auto* t = params.registry->getTransform(params.fingBuilding);
            auto* mg = params.registry->getMeshGroup(params.fingBuilding);
            if (t && mg) {
                m_config.depthShader->use();
                m_config.depthShader->setMat4("uLightSpaceMatrix", lightSpaceMatrix);
                m_config.depthShader->setMat4("uModel", t->matrix());
                for (const auto& mesh : mg->meshes) {
                    glBindVertexArray(mesh.vao);
                    glDrawElements(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr);
                }
            }
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);

        return lightSpaceMatrix;
    }

    void renderBuildings(const FrameParams& params, const glm::mat4& lightSpaceMatrix) {
        // Update building culling
        params.buildingCuller->update(params.view, params.projection, params.cameraPos,
                                       params.buildingMaxRenderDistance);

        BuildingRenderParams buildingParams;
        buildingParams.view = params.view;
        buildingParams.projection = params.projection;
        buildingParams.lightSpaceMatrix = lightSpaceMatrix;
        buildingParams.lightDir = m_config.lightDir;
        buildingParams.viewPos = params.cameraPos;
        buildingParams.texture = m_config.brickTexture;
        buildingParams.normalMap = m_config.brickNormalMap;
        buildingParams.shadowMap = m_config.shadowDepthTexture;
        buildingParams.textureScale = GameConfig::BUILDING_TEXTURE_SCALE;
        buildingParams.fogEnabled = params.gameState->fogEnabled;
        buildingParams.shadowsEnabled = true;

        params.buildingCuller->render(*m_config.buildingBoxMesh, *m_config.buildingInstancedShader,
                                       buildingParams);
    }

    void renderSun(const FrameParams& params) {
        glm::vec3 sunWorldPos = params.cameraPos + m_config.lightDir * 400.0f;
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        m_config.sunShader->use();
        m_config.sunShader->setMat4("uView", params.view);
        m_config.sunShader->setMat4("uProjection", params.projection);
        m_config.sunShader->setVec3("uSunWorldPos", sunWorldPos);
        m_config.sunShader->setFloat("uSize", 30.0f);

        glBindVertexArray(m_config.sunVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
        glBindVertexArray(0);

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }

    void renderComets(const FrameParams& params) {
        if (!m_config.cometMeshGroup) return;

        glEnable(GL_DEPTH_TEST);
        glDepthMask(GL_FALSE);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        m_config.cometShader->use();
        m_config.cometShader->setMat4("uView", params.view);
        m_config.cometShader->setMat4("uProjection", params.projection);
        m_config.cometShader->setFloat("uTime", params.gameState->gameTime);
        m_config.cometShader->setVec3("uCameraPos", params.cameraPos);
        m_config.cometShader->setFloat("uFallSpeed", m_config.cometFallSpeed);
        m_config.cometShader->setFloat("uCycleTime", m_config.cometCycleTime);
        m_config.cometShader->setFloat("uFallDistance", m_config.cometFallDistance);
        m_config.cometShader->setVec3("uFallDirection", m_config.cometFallDir);
        m_config.cometShader->setFloat("uScale", m_config.cometScale);
        m_config.cometShader->setVec3("uCometColor", m_config.cometColor);
        m_config.cometShader->setInt("uDebugMode", 0);
        m_config.cometShader->setInt("uTexture", 0);
        m_config.cometShader->setFloat("uTrailStretch", 15.0f);
        m_config.cometShader->setFloat("uGroundY", 0.0f);

        for (const auto& mesh : m_config.cometMeshGroup->meshes) {
            if (mesh.texture != 0) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, mesh.texture);
                m_config.cometShader->setInt("uHasTexture", 1);
            } else {
                m_config.cometShader->setInt("uHasTexture", 0);
            }

            glBindVertexArray(mesh.vao);
            glDrawElementsInstanced(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr,
                                     m_config.numComets);
        }
        glBindVertexArray(0);

        glDisable(GL_BLEND);
        glDepthMask(GL_TRUE);
        glEnable(GL_DEPTH_TEST);
    }
};
