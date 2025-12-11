#pragma once
#include "../IScene.h"
#include "../SceneContext.h"
#include "../SceneManager.h"
#include "../RenderHelpers.h"
#include "../../ecs/Registry.h"
#include "../../ecs/systems/AnimationSystem.h"
#include "../../ecs/systems/SkeletonSystem.h"
#include "../../ecs/systems/CinematicSystem.h"
#include "../../ecs/systems/RenderSystem.h"
#include "../../core/GameState.h"
#include "../../core/GameConfig.h"
#include "../../culling/BuildingCuller.h"
#include "../../rendering/RenderPipeline.h"
#include "../../Shader.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

class IntroCinematicScene : public IScene {
public:
    void onEnter(SceneContext& ctx) override {
        // Reset protagonist position
        auto* pt = ctx.registry->getTransform(ctx.protagonist);
        if (pt) {
            pt->position = GameConfig::INTRO_CHARACTER_POS;
        }

        // Character faces toward FING
        auto* pf = ctx.registry->getFacingDirection(ctx.protagonist);
        if (pf) {
            pf->yaw = 225.0f;
        }

        // Reset motion blur accumulation
        ctx.gameState->motionBlurInitialized = false;
        ctx.gameState->motionBlurPingPong = 0;

        // Start the cinematic
        ctx.cinematicSystem->start(*ctx.registry);
    }

    void update(SceneContext& ctx) override {
        // Skip cinematic with Enter or Escape
        if (ctx.input.enterPressed || ctx.input.escapePressed) {
            ctx.cinematicSystem->stop(*ctx.registry);
            ctx.sceneManager->switchTo(SceneType::PlayGame);
            return;
        }

        // Update cinematic - when complete, switch to gameplay
        if (!ctx.cinematicSystem->update(*ctx.registry, ctx.dt)) {
            if (ctx.cinematicSystem->isComplete()) {
                ctx.sceneManager->switchTo(SceneType::PlayGame);
                return;
            }
        }

        // Update animations for visual effect
        ctx.animationSystem->update(*ctx.registry, ctx.dt);
        ctx.skeletonSystem->update(*ctx.registry);
    }

    void render(SceneContext& ctx) override {
        auto* protagonistT = ctx.registry->getTransform(ctx.protagonist);
        auto* cam = ctx.registry->getCamera(ctx.camera);
        auto* camT = ctx.registry->getTransform(ctx.camera);

        glm::mat4 cinematicView = ctx.cinematicSystem->getViewMatrix(*ctx.registry);
        glm::mat4 projection = cam ? cam->projectionMatrix(ctx.aspectRatio) : glm::mat4(1.0f);
        glm::vec3 cameraPos = ctx.cinematicSystem->getCurrentCameraPosition();

        // Update building culling
        ctx.buildingCuller->update(cinematicView, projection, cameraPos, ctx.buildingMaxRenderDistance);

        // Compute current view-projection for motion blur
        glm::mat4 currentViewProjection = projection * cinematicView;

        // === SHADOW PASS ===
        glm::vec3 focusPoint = protagonistT ? protagonistT->position : glm::vec3(0.0f);
        glm::mat4 lightSpaceMatrix = RenderHelpers::computeLightSpaceMatrix(focusPoint, ctx.lightDir);

        ctx.renderPipeline->beginShadowPass();
        ctx.renderPipeline->renderShadowCasters(lightSpaceMatrix, cameraPos);
        ctx.renderPipeline->endShadowPass();

        // === RENDER TO CINEMATIC MSAA FBO ===
        ctx.renderPipeline->beginCinematicPass();

        // Debug axes
        if (cam && camT && ctx.axes) {
            glm::mat4 vp = projection * cinematicView;
            ctx.colorShader->use();
            ctx.colorShader->setMat4("uMVP", vp);
            ctx.axes->draw();
        }

        // Render scene with shadows
        RenderHelpers::setupRenderSystem(*ctx.renderSystem, ctx.gameState->fogEnabled, true,
                                          ctx.shadowDepthTexture, lightSpaceMatrix);
        ctx.renderSystem->updateWithView(*ctx.registry, ctx.aspectRatio, cinematicView);

        // Render buildings
        BuildingRenderParams params;
        params.view = cinematicView;
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
        ctx.renderPipeline->renderBuildings(params);

        // Render ground plane
        RenderHelpers::renderGroundPlane(*ctx.groundShader, cinematicView, projection, lightSpaceMatrix,
            ctx.lightDir, cameraPos, ctx.gameState->fogEnabled, true,
            ctx.snowTexture, ctx.shadowDepthTexture, ctx.planeVAO);

        // Render snow overlay
        RenderHelpers::renderSnowOverlay(*ctx.overlayShader, ctx.overlayVAO, *ctx.gameState);

        // Render comets
        ctx.renderPipeline->renderComets(cinematicView, projection, cameraPos);

        // === MOTION BLUR POST-PROCESS ===
        ctx.renderPipeline->applyMotionBlur(currentViewProjection,
            *ctx.prevViewProjection, ctx.gameState->motionBlurInitialized);

        // === FINAL RESOLVE AND BLIT ===
        ctx.renderPipeline->finalResolveAndBlit();
    }

    void onExit(SceneContext& ctx) override {
        // Nothing special needed
    }
};
