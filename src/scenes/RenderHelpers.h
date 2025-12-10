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

// Execute shadow pass for building entities
inline void renderShadowPass(GLuint shadowFBO, int shadowSize,
                             Shader& depthShader,
                             const glm::mat4& lightSpaceMatrix,
                             Registry& registry,
                             const std::vector<Entity>& buildingEntityPool) {
    glViewport(0, 0, shadowSize, shadowSize);
    glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);

    depthShader.use();
    depthShader.setMat4("uLightSpaceMatrix", lightSpaceMatrix);

    for (Entity e : buildingEntityPool) {
        auto* t = registry.getTransform(e);
        if (t && t->position.y > -100.0f) {
            depthShader.setMat4("uModel", t->matrix());
            auto* mg = registry.getMeshGroup(e);
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

// Update building culling based on player position
inline void updateBuildingCulling(Registry& registry,
                                  GameState& gameState,
                                  const glm::vec3& playerPos,
                                  const std::vector<BuildingGenerator::BuildingData>& buildingDataList,
                                  std::vector<Entity>& buildingEntityPool,
                                  int buildingRenderRadius) {
    auto [playerGridX, playerGridZ] = BuildingGenerator::getPlayerGridCell(playerPos);

    if (playerGridX != gameState.lastPlayerGridX || playerGridZ != gameState.lastPlayerGridZ) {
        gameState.lastPlayerGridX = playerGridX;
        gameState.lastPlayerGridZ = playerGridZ;

        std::vector<const BuildingGenerator::BuildingData*> visibleBuildings;
        for (const auto& bldg : buildingDataList) {
            if (BuildingGenerator::isBuildingInRange(bldg, playerGridX, playerGridZ, buildingRenderRadius)) {
                visibleBuildings.push_back(&bldg);
            }
        }

        size_t entityIdx = 0;
        for (const auto* bldg : visibleBuildings) {
            if (entityIdx >= buildingEntityPool.size()) break;
            auto* transform = registry.getTransform(buildingEntityPool[entityIdx]);
            if (transform) {
                transform->position = bldg->position;
                transform->scale = glm::vec3(bldg->width, bldg->height, bldg->depth);
            }
            ++entityIdx;
        }
        // Hide remaining buildings
        for (; entityIdx < buildingEntityPool.size(); ++entityIdx) {
            auto* transform = registry.getTransform(buildingEntityPool[entityIdx]);
            if (transform) {
                transform->position.y = -1000.0f;
            }
        }
    }
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
