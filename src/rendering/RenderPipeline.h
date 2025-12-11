#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "../core/GameConfig.h"
#include "../core/GameState.h"
#include "../culling/BuildingCuller.h"
#include "../Shader.h"
#include "../ecs/Registry.h"
#include "../ecs/components/Mesh.h"
#include "../ecs/components/Skeleton.h"

// Forward declaration to avoid circular include
struct SceneContext;

// Centralizes FBO management and post-processing effects
// Eliminates duplicated rendering code across scenes
class RenderPipeline {
public:
    void init(SceneContext* ctx) {
        m_ctx = ctx;
    }

    // ==================== FBO Management ====================

    void beginShadowPass();
    void endShadowPass();
    void beginMainPass(bool useToonFBO = false);
    void beginCinematicPass();

    // ==================== Post-Processing ====================

    void applyToonPostProcess();
    void applyMotionBlur(const glm::mat4& currentVP, glm::mat4& prevVP, bool& initialized);
    void finalResolveAndBlit();

    // ==================== Common Rendering Helpers ====================

    void renderShadowCasters(const glm::mat4& lightSpaceMatrix, const glm::vec3& cameraPos);
    void renderBuildings(const BuildingRenderParams& params);
    void renderComets(const glm::mat4& view, const glm::mat4& projection,
                      const glm::vec3& cameraPos, const glm::vec3& fallDir,
                      const glm::vec3& cometColor);
    void renderComets(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos);
    void renderSun(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos);
    void renderSnow(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& playerPos);

    // ==================== Debug ====================
    void renderShadowMapDebug();

private:
    SceneContext* m_ctx = nullptr;
};

// Include SceneContext after class declaration to avoid circular dependency
#include "../scenes/SceneContext.h"

// ==================== Implementation ====================

inline void RenderPipeline::beginShadowPass() {
    glViewport(0, 0, GameConfig::SHADOW_MAP_SIZE, GameConfig::SHADOW_MAP_SIZE);
    glBindFramebuffer(GL_FRAMEBUFFER, m_ctx->shadowFBO);
    glClear(GL_DEPTH_BUFFER_BIT);
}

inline void RenderPipeline::endShadowPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
}

inline void RenderPipeline::beginMainPass(bool useToonFBO) {
    GLuint targetFBO = useToonFBO ? m_ctx->toonFBO : m_ctx->msaaFBO;
    glBindFramebuffer(GL_FRAMEBUFFER, targetFBO);
    glViewport(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
    glClearColor(0.2f, 0.2f, 0.22f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

inline void RenderPipeline::beginCinematicPass() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_ctx->cinematicMsaaFBO);
    glViewport(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
    glClearColor(0.2f, 0.2f, 0.22f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

inline void RenderPipeline::applyToonPostProcess() {
    glBindFramebuffer(GL_FRAMEBUFFER, m_ctx->msaaFBO);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    m_ctx->toonPostShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_ctx->toonColorTex);
    m_ctx->toonPostShader->setInt("uSceneTex", 0);
    m_ctx->toonPostShader->setVec2("uTexelSize", glm::vec2(
        1.0f / GameConfig::WINDOW_WIDTH,
        1.0f / GameConfig::WINDOW_HEIGHT));

    glDisable(GL_DEPTH_TEST);
    glBindVertexArray(m_ctx->overlayVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
    glEnable(GL_DEPTH_TEST);
}

inline void RenderPipeline::applyMotionBlur(const glm::mat4& currentVP, glm::mat4& prevVP, bool& initialized) {
    // Resolve cinematic MSAA to motion blur FBO
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_ctx->cinematicMsaaFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ctx->motionBlurFBO);
    glBlitFramebuffer(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT,
                      0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT,
                      GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

    // Apply motion blur effect to msaaFBO
    glBindFramebuffer(GL_FRAMEBUFFER, m_ctx->msaaFBO);
    glViewport(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    m_ctx->motionBlurShader->use();

    // Bind color buffer
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_ctx->motionBlurColorTex);
    m_ctx->motionBlurShader->setInt("uColorBuffer", 0);

    // Bind depth buffer
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_ctx->motionBlurDepthTex);
    m_ctx->motionBlurShader->setInt("uDepthBuffer", 1);

    // Pass matrices for velocity computation
    m_ctx->motionBlurShader->setMat4("uViewProjection", currentVP);
    m_ctx->motionBlurShader->setMat4("uInvViewProjection", glm::inverse(currentVP));

    // On first frame, use current matrix as previous (no blur)
    if (!initialized) {
        prevVP = currentVP;
    }
    m_ctx->motionBlurShader->setMat4("uPrevViewProjection", prevVP);

    // Blur parameters
    m_ctx->motionBlurShader->setFloat("uBlurStrength", GameConfig::CINEMATIC_MOTION_BLUR);
    m_ctx->motionBlurShader->setInt("uNumSamples", 16);

    glBindVertexArray(m_ctx->overlayVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);

    // Store current view-projection for next frame
    prevVP = currentVP;
    initialized = true;
}

inline void RenderPipeline::finalResolveAndBlit() {
    // Step 1: Resolve MSAA FBO to regular texture FBO
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_ctx->msaaFBO);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_ctx->resolveFBO);
    glBlitFramebuffer(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT,
                      0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT,
                      GL_COLOR_BUFFER_BIT, GL_LINEAR);

    // Step 2: Blit resolved texture to screen
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    m_ctx->blitShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_ctx->resolveColorTex);
    m_ctx->blitShader->setInt("uScreenTexture", 0);

    glBindVertexArray(m_ctx->overlayVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glEnable(GL_DEPTH_TEST);
}

inline void RenderPipeline::renderShadowCasters(const glm::mat4& lightSpaceMatrix, const glm::vec3& cameraPos) {
    // Render buildings to shadow map
    m_ctx->buildingCuller->updateShadowCasters(lightSpaceMatrix, cameraPos, m_ctx->buildingMaxRenderDistance);
    m_ctx->buildingCuller->renderShadows(*m_ctx->buildingBoxMesh, *m_ctx->depthInstancedShader, lightSpaceMatrix);

    // Render FING building shadow
    auto* t = m_ctx->registry->getTransform(m_ctx->fingBuilding);
    auto* mg = m_ctx->registry->getMeshGroup(m_ctx->fingBuilding);
    if (t && mg) {
        m_ctx->depthShader->use();
        m_ctx->depthShader->setMat4("uLightSpaceMatrix", lightSpaceMatrix);
        m_ctx->depthShader->setMat4("uModel", t->matrix());
        for (const auto& mesh : mg->meshes) {
            glBindVertexArray(mesh.vao);
            glDrawElements(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr);
        }
    }

    // Render protagonist shadow (skinned mesh)
    auto* protagonistT = m_ctx->registry->getTransform(m_ctx->protagonist);
    auto* protagonistMG = m_ctx->registry->getMeshGroup(m_ctx->protagonist);
    auto* protagonistSkeleton = m_ctx->registry->getSkeleton(m_ctx->protagonist);
    auto* protagonistR = m_ctx->registry->getRenderable(m_ctx->protagonist);
    if (protagonistT && protagonistMG) {
        m_ctx->skinnedDepthShader->use();
        m_ctx->skinnedDepthShader->setMat4("uLightSpaceMatrix", lightSpaceMatrix);

        // Apply mesh offset to match render system
        glm::mat4 model = protagonistT->matrix();
        if (protagonistR && protagonistR->meshOffset != glm::vec3(0.0f)) {
            model = model * glm::translate(glm::mat4(1.0f), protagonistR->meshOffset);
        }
        m_ctx->skinnedDepthShader->setMat4("uModel", model);

        // Set skinning data
        bool hasSkinning = protagonistSkeleton && !protagonistSkeleton->boneMatrices.empty();
        m_ctx->skinnedDepthShader->setInt("uUseSkinning", hasSkinning ? 1 : 0);
        if (hasSkinning) {
            m_ctx->skinnedDepthShader->setMat4Array("uBones", protagonistSkeleton->boneMatrices);
        }

        for (const auto& mesh : protagonistMG->meshes) {
            glBindVertexArray(mesh.vao);
            glDrawElements(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr);
        }
    }

    // Render NPC shadows (skinned meshes)
    for (Entity npc : m_ctx->npcs) {
        auto* npcT = m_ctx->registry->getTransform(npc);
        auto* npcMG = m_ctx->registry->getMeshGroup(npc);
        auto* npcSkeleton = m_ctx->registry->getSkeleton(npc);
        auto* npcR = m_ctx->registry->getRenderable(npc);
        if (npcT && npcMG) {
            m_ctx->skinnedDepthShader->use();
            m_ctx->skinnedDepthShader->setMat4("uLightSpaceMatrix", lightSpaceMatrix);

            // Apply mesh offset to match render system
            glm::mat4 model = npcT->matrix();
            if (npcR && npcR->meshOffset != glm::vec3(0.0f)) {
                model = model * glm::translate(glm::mat4(1.0f), npcR->meshOffset);
            }
            m_ctx->skinnedDepthShader->setMat4("uModel", model);

            bool hasSkinning = npcSkeleton && !npcSkeleton->boneMatrices.empty();
            m_ctx->skinnedDepthShader->setInt("uUseSkinning", hasSkinning ? 1 : 0);
            if (hasSkinning) {
                m_ctx->skinnedDepthShader->setMat4Array("uBones", npcSkeleton->boneMatrices);
            }

            for (const auto& mesh : npcMG->meshes) {
                glBindVertexArray(mesh.vao);
                glDrawElements(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr);
            }
        }
    }
}

inline void RenderPipeline::renderBuildings(const BuildingRenderParams& params) {
    m_ctx->buildingCuller->render(*m_ctx->buildingBoxMesh, *m_ctx->buildingInstancedShader, params);
}

inline void RenderPipeline::renderComets(const glm::mat4& view, const glm::mat4& projection,
                                          const glm::vec3& cameraPos, const glm::vec3& fallDir,
                                          const glm::vec3& cometColor) {
    if (!m_ctx->cometMeshGroup) return;

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_ctx->cometShader->use();
    m_ctx->cometShader->setMat4("uView", view);
    m_ctx->cometShader->setMat4("uProjection", projection);
    m_ctx->cometShader->setFloat("uTime", m_ctx->gameState->gameTime);
    m_ctx->cometShader->setVec3("uCameraPos", cameraPos);
    m_ctx->cometShader->setFloat("uFallSpeed", m_ctx->cometFallSpeed);
    m_ctx->cometShader->setFloat("uCycleTime", m_ctx->cometCycleTime);
    m_ctx->cometShader->setFloat("uFallDistance", m_ctx->cometFallDistance);
    m_ctx->cometShader->setVec3("uFallDirection", fallDir);
    m_ctx->cometShader->setFloat("uScale", m_ctx->cometScale);
    m_ctx->cometShader->setVec3("uCometColor", cometColor);
    m_ctx->cometShader->setInt("uDebugMode", 0);
    m_ctx->cometShader->setInt("uTexture", 0);
    m_ctx->cometShader->setFloat("uTrailStretch", 15.0f);
    m_ctx->cometShader->setFloat("uGroundY", 0.0f);

    for (const auto& mesh : m_ctx->cometMeshGroup->meshes) {
        if (mesh.texture != 0) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, mesh.texture);
            m_ctx->cometShader->setInt("uHasTexture", 1);
        } else {
            m_ctx->cometShader->setInt("uHasTexture", 0);
        }

        glBindVertexArray(mesh.vao);
        glDrawElementsInstanced(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr, m_ctx->numComets);
    }
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
    glEnable(GL_DEPTH_TEST);
}

inline void RenderPipeline::renderComets(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {
    renderComets(view, projection, cameraPos, m_ctx->cometFallDir, m_ctx->cometColor);
}

inline void RenderPipeline::renderSun(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& cameraPos) {
    glm::vec3 sunWorldPos = cameraPos + m_ctx->lightDir * 400.0f;

    // Enable depth test so sun is occluded by buildings, but don't write to depth buffer
    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_ctx->sunShader->use();
    m_ctx->sunShader->setMat4("uView", view);
    m_ctx->sunShader->setMat4("uProjection", projection);
    m_ctx->sunShader->setVec3("uSunWorldPos", sunWorldPos);
    m_ctx->sunShader->setFloat("uSize", 30.0f);

    glBindVertexArray(m_ctx->sunVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}

inline void RenderPipeline::renderShadowMapDebug() {
    if (!GameConfig::SHOW_SHADOW_MAP) return;

    // Draw shadow map in bottom-left corner (256x256)
    const int debugSize = 256;
    glViewport(10, 10, debugSize, debugSize);
    glDisable(GL_DEPTH_TEST);

    // Use blit shader to display depth texture
    m_ctx->blitShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_ctx->shadowDepthTexture);
    m_ctx->blitShader->setInt("uScreenTexture", 0);

    glBindVertexArray(m_ctx->overlayVAO);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);

    // Restore viewport
    glViewport(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);
}

inline void RenderPipeline::renderSnow(const glm::mat4& view, const glm::mat4& projection, const glm::vec3& playerPos) {
    if (!m_ctx->snowShader || m_ctx->snowVAO == 0 || m_ctx->snowParticleCount == 0) return;

    glEnable(GL_DEPTH_TEST);
    glDepthMask(GL_FALSE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    m_ctx->snowShader->use();
    m_ctx->snowShader->setMat4("uView", view);
    m_ctx->snowShader->setMat4("uProjection", projection);
    m_ctx->snowShader->setVec3("uPlayerPos", playerPos);
    m_ctx->snowShader->setFloat("uTime", m_ctx->gameState->gameTime);
    m_ctx->snowShader->setFloat("uSphereRadius", GameConfig::SNOW_SPHERE_RADIUS);
    m_ctx->snowShader->setFloat("uFallSpeed", GameConfig::SNOW_PARTICLE_FALL_SPEED);
    m_ctx->snowShader->setFloat("uParticleSize", GameConfig::SNOW_PARTICLE_SIZE);
    m_ctx->snowShader->setFloat("uWindStrength", GameConfig::SNOW_WIND_STRENGTH);
    m_ctx->snowShader->setFloat("uWindAngle", glm::radians(m_ctx->gameState->snowAngle));

    glBindVertexArray(m_ctx->snowVAO);
    glDrawArraysInstanced(GL_TRIANGLE_STRIP, 0, 4, m_ctx->snowParticleCount);
    glBindVertexArray(0);

    glDisable(GL_BLEND);
    glDepthMask(GL_TRUE);
}
