#pragma once
#include "../IScene.h"
#include "../SceneContext.h"
#include "../SceneManager.h"
#include "../RenderHelpers.h"
#include "../../ecs/Registry.h"
#include "../../ecs/systems/AnimationSystem.h"
#include "../../ecs/systems/SkeletonSystem.h"
#include "../../ecs/systems/FollowCameraSystem.h"
#include "../../ecs/systems/RenderSystem.h"
#include "../../systems/MonsterManager.h"
#include "../../ecs/components/MonsterData.h"
#include "../../core/GameState.h"
#include "../../core/GameConfig.h"
#include "../../culling/BuildingCuller.h"
#include "../../rendering/RenderPipeline.h"
#include "../../Shader.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

class DeathCinematicScene : public IScene {
public:
    void onEnter(SceneContext& ctx) override {
        // Reset motion blur accumulation
        ctx.gameState->motionBlurInitialized = false;
        ctx.gameState->motionBlurPingPong = 0;

        // Reset elapsed time
        m_elapsedTime = 0.0f;

        // Calculate cinematic duration based on distance
        // Monster travels at CHASE_SPEED, but we apply slow-mo to game time
        // Real duration = (distance / chase_speed) / slow_mo_factor
        // This way, monster arrives exactly when cinematic ends
        float distance = ctx.deathCinematicDistance;
        m_cinematicDuration = (distance / MonsterData::CHASE_SPEED) / SLOW_MO_FACTOR;

        // Clamp to reasonable bounds (min 1.5s, max 5s)
        m_cinematicDuration = glm::clamp(m_cinematicDuration, 1.5f, 5.0f);

        // Player is frozen - no position changes
        // Monster continues to chase in slow motion
    }

    void update(SceneContext& ctx) override {
        // Slow motion: game time passes slower
        float slowDt = ctx.dt * SLOW_MO_FACTOR;

        // Increment elapsed time (real time for duration check)
        m_elapsedTime += ctx.dt;

        // When duration elapsed, transition to YouDied
        if (m_elapsedTime >= m_cinematicDuration) {
            ctx.sceneManager->switchTo(SceneType::YouDied);
            return;
        }

        // Continue monster AI in slow motion (chasing toward frozen player)
        auto* protagonistT = ctx.registry->getTransform(ctx.protagonist);
        if (ctx.monsterManager && protagonistT) {
            ctx.monsterManager->update(slowDt, protagonistT->position);
        }

        // Update animations in slow motion for dramatic effect
        ctx.animationSystem->update(*ctx.registry, slowDt);
        ctx.skeletonSystem->update(*ctx.registry);
    }

    void render(SceneContext& ctx) override {
        auto* protagonistT = ctx.registry->getTransform(ctx.protagonist);
        auto* protagonistFacing = ctx.registry->getFacingDirection(ctx.protagonist);
        auto* cam = ctx.registry->getCamera(ctx.camera);
        auto* camT = ctx.registry->getTransform(ctx.camera);
        auto* ft = ctx.registry->getFollowTarget(ctx.camera);

        // Use follow camera view (player's perspective)
        glm::mat4 view = glm::mat4(1.0f);
        glm::vec3 cameraPos = glm::vec3(0.0f);

        if (cam && camT && protagonistT && protagonistFacing && ft) {
            glm::vec3 lookAtPos = FollowCameraSystem::getLookAtPosition(*protagonistT, *ft, protagonistFacing->yaw);
            view = glm::lookAt(camT->position, lookAtPos, glm::vec3(0.0f, 1.0f, 0.0f));
            cameraPos = camT->position;
        }

        glm::mat4 projection = cam ? cam->projectionMatrix(ctx.aspectRatio) : glm::mat4(1.0f);

        // Update building culling
        ctx.buildingCuller->update(view, projection, cameraPos, ctx.buildingMaxRenderDistance);

        // Compute current view-projection for motion blur
        glm::mat4 currentViewProjection = projection * view;

        // === SHADOW PASS ===
        glm::vec3 focusPoint = protagonistT ? protagonistT->position : glm::vec3(0.0f);
        glm::mat4 lightSpaceMatrix = RenderHelpers::computeLightSpaceMatrix(focusPoint, ctx.lightDir);

        ctx.renderPipeline->beginShadowPass();
        ctx.renderPipeline->renderShadowCasters(lightSpaceMatrix, cameraPos);
        ctx.renderPipeline->endShadowPass();

        // === RENDER TO CINEMATIC MSAA FBO ===
        ctx.renderPipeline->beginCinematicPass();

        // Render scene with shadows
        RenderHelpers::setupRenderSystem(*ctx.renderSystem, ctx.gameState->fogEnabled, true,
                                          ctx.shadowDepthTexture, lightSpaceMatrix);
        ctx.renderSystem->setFogDensity(GameConfig::FOG_DENSITY);
        ctx.renderSystem->setFogColor(GameConfig::FOG_COLOR);
        ctx.renderSystem->updateWithView(*ctx.registry, ctx.aspectRatio, view);

        // Render buildings
        BuildingRenderParams params;
        params.view = view;
        params.projection = projection;
        params.lightSpaceMatrix = lightSpaceMatrix;
        params.lightDir = ctx.lightDir;
        params.viewPos = cameraPos;
        params.texture = ctx.brickTexture;
        params.normalMap = ctx.brickNormalMap;
        params.shadowMap = ctx.shadowDepthTexture;
        params.textureScale = GameConfig::BUILDING_TEXTURE_SCALE;
        params.fogEnabled = ctx.gameState->fogEnabled;
        params.shadowsEnabled = true;
        params.fogColor = GameConfig::FOG_COLOR;
        params.fogDensity = GameConfig::FOG_DENSITY;
        ctx.renderPipeline->renderBuildings(params);

        // Render ground plane
        RenderHelpers::renderGroundPlane(*ctx.groundShader, view, projection, lightSpaceMatrix,
            ctx.lightDir, cameraPos, ctx.gameState->fogEnabled, true,
            ctx.snowTexture, ctx.shadowDepthTexture, ctx.planeVAO,
            GameConfig::FOG_DENSITY, GameConfig::FOG_COLOR);

        // Render monster danger zones
        if (ctx.monsterManager) {
            std::vector<glm::vec3> dangerZonePositions = ctx.monsterManager->getPositions();
            ctx.renderPipeline->renderDangerZones(view, projection, dangerZonePositions, MonsterData::DETECTION_RADIUS);
        }

        // Render sun
        ctx.renderPipeline->renderSun(view, projection, cameraPos);

        // Render comets
        ctx.renderPipeline->renderComets(view, projection, cameraPos);

        // Render 3D snow particles
        if (protagonistT) {
            ctx.renderPipeline->renderSnow(view, projection, protagonistT->position);
        }

        // Render snow overlay
        RenderHelpers::renderSnowOverlay(*ctx.overlayShader, ctx.overlayVAO, *ctx.gameState);

        // === RADIAL BLUR POST-PROCESS (dramatic tunnel vision effect) ===
        ctx.renderPipeline->applyRadialBlur(DEATH_BLUR_STRENGTH);

        // === FINAL RESOLVE AND BLIT ===
        ctx.renderPipeline->finalResolveAndBlit();
    }

    void onExit(SceneContext& ctx) override {
        // Nothing special needed
    }

private:
    static constexpr float SLOW_MO_FACTOR = 0.2f;       // 5x slower
    static constexpr float DEATH_BLUR_STRENGTH = 8.0f;  // Very exaggerated motion blur

    float m_elapsedTime = 0.0f;
    float m_cinematicDuration = 2.5f;  // Calculated based on distance
};
