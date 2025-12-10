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
#include "../procedural/BuildingGenerator.h"
#include <vector>

namespace RenderHelpers {

// Compute light space matrix for shadow mapping
inline glm::mat4 computeLightSpaceMatrix(const glm::vec3& focusPoint, const glm::vec3& lightDir) {
    float orthoSize = GameConfig::SHADOW_ORTHO_SIZE;
    glm::vec3 lightPos = focusPoint + lightDir * GameConfig::SHADOW_DISTANCE;
    glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize,
                                            GameConfig::SHADOW_NEAR, GameConfig::SHADOW_FAR);
    glm::mat4 lightView = glm::lookAt(lightPos, focusPoint, glm::vec3(0.0f, 1.0f, 0.0f));
    return lightProjection * lightView;
}

// Render ground plane with all uniforms
inline void renderGroundPlane(Shader& groundShader,
                              const glm::mat4& view, const glm::mat4& projection,
                              const glm::mat4& lightSpaceMatrix,
                              const glm::vec3& lightDir, const glm::vec3& viewPos,
                              bool fogEnabled, bool shadowsEnabled,
                              GLuint snowTexture, GLuint shadowDepthTexture,
                              GLuint planeVAO) {
    groundShader.use();
    groundShader.setMat4("uView", view);
    groundShader.setMat4("uProjection", projection);
    groundShader.setMat4("uModel", glm::mat4(1.0f));
    groundShader.setMat4("uLightSpaceMatrix", lightSpaceMatrix);
    groundShader.setVec3("uLightDir", lightDir);
    groundShader.setVec3("uViewPos", viewPos);
    groundShader.setInt("uHasTexture", 1);
    groundShader.setInt("uFogEnabled", fogEnabled ? 1 : 0);
    groundShader.setInt("uShadowsEnabled", shadowsEnabled ? 1 : 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, snowTexture);
    groundShader.setInt("uTexture", 0);
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);
    groundShader.setInt("uShadowMap", 1);

    glBindVertexArray(planeVAO);
    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
    glBindVertexArray(0);
}

// Render snow overlay effect
inline void renderSnowOverlay(Shader& overlayShader, GLuint overlayVAO,
                              const GameState& gameState) {
    if (!gameState.snowEnabled) return;

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    overlayShader.use();
    overlayShader.setVec3("iResolution",
        glm::vec3((float)GameConfig::WINDOW_WIDTH, (float)GameConfig::WINDOW_HEIGHT, 1.0f));
    overlayShader.setFloat("iTime", gameState.gameTime);
    overlayShader.setFloat("uSnowSpeed", gameState.snowSpeed);
    overlayShader.setFloat("uSnowDirectionDeg", gameState.snowAngle);
    overlayShader.setFloat("uMotionBlur", gameState.snowMotionBlur);

    glBindVertexArray(overlayVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glEnable(GL_DEPTH_TEST);
}

// Update FING building LOD based on distance
inline void updateFingLOD(Registry& registry, GameState& gameState,
                          Entity fingBuilding,
                          const glm::vec3& viewerPos,
                          MeshGroup& highDetail, MeshGroup& lowDetail,
                          float lodSwitchDistance) {
    auto* fingT = registry.getTransform(fingBuilding);
    if (!fingT) return;

    float dist = glm::length(viewerPos - fingT->position);
    bool shouldUseHighDetail = dist < lodSwitchDistance;

    if (shouldUseHighDetail != gameState.fingUsingHighDetail) {
        gameState.fingUsingHighDetail = shouldUseHighDetail;
        auto* meshGroup = registry.getMeshGroup(fingBuilding);
        if (meshGroup) {
            meshGroup->meshes = gameState.fingUsingHighDetail ? highDetail.meshes : lowDetail.meshes;
        }
    }
}

// Setup render system for shadow rendering
inline void setupRenderSystem(RenderSystem& renderSystem,
                              bool fogEnabled, bool shadowsEnabled,
                              GLuint shadowDepthTexture,
                              const glm::mat4& lightSpaceMatrix) {
    renderSystem.setFogEnabled(fogEnabled);
    renderSystem.setShadowsEnabled(shadowsEnabled);
    renderSystem.setShadowMap(shadowDepthTexture);
    renderSystem.setLightSpaceMatrix(lightSpaceMatrix);
}

}  // namespace RenderHelpers
