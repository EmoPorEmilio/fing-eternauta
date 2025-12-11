#pragma once
#include "../IScene.h"
#include "../SceneContext.h"
#include "../SceneManager.h"
#include "../RenderHelpers.h"
#include "../../ecs/Registry.h"
#include "../../ecs/systems/RenderSystem.h"
#include "../../ecs/systems/UISystem.h"
#include "../../ecs/systems/MinimapSystem.h"
#include "../../ecs/systems/CameraOrbitSystem.h"
#include "../../ecs/systems/FollowCameraSystem.h"
#include "../../ecs/systems/PlayerMovementSystem.h"
#include "../../ecs/systems/PhysicsSystem.h"
#include "../../ecs/systems/CollisionSystem.h"
#include "../../ecs/systems/AnimationSystem.h"
#include "../../ecs/systems/SkeletonSystem.h"
#include "../../systems/MonsterManager.h"
#include "../../ecs/components/MonsterData.h"
#include "../../core/GameState.h"
#include "../../core/GameConfig.h"
#include "../../culling/BuildingCuller.h"
#include "../../rendering/RenderPipeline.h"
#include "../../Shader.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

class PlayGameScene : public IScene {
public:
    void onEnter(SceneContext& ctx) override {
        ctx.inputSystem->captureMouse(true);

        // Show sprint hint
        ctx.registry->getUIText(ctx.sprintHint)->visible = true;

        // Only reset game state when NOT coming from pause menu
        if (ctx.sceneManager->previous() != SceneType::PauseMenu) {
            auto* pt = ctx.registry->getTransform(ctx.protagonist);
            if (pt) {
                pt->position = GameConfig::INTRO_CHARACTER_POS;
            }
            auto* pf = ctx.registry->getFacingDirection(ctx.protagonist);
            if (pf) {
                pf->yaw = 225.0f;
            }

            // Reset protagonist animation to idle
            auto* anim = ctx.registry->getAnimation(ctx.protagonist);
            if (anim) {
                anim->clipIndex = 0;  // Idle animation
                anim->time = 0.0f;
                anim->speedMultiplier = 1.0f;
            }

            // Reset all monsters to their patrol positions
            if (ctx.monsterManager) {
                ctx.monsterManager->resetAll();
            }
        }
    }

    void update(SceneContext& ctx) override {
        // Open pause menu on escape
        if (ctx.input.escapePressed) {
            ctx.sceneManager->switchTo(SceneType::PauseMenu);
            return;
        }

        // Update gameplay systems
        ctx.cameraOrbitSystem->update(*ctx.registry, ctx.input.mouseX, ctx.input.mouseY);

        // Player movement with building collision
        ctx.playerMovementSystem->update(*ctx.registry, ctx.dt, ctx.buildingCuller, nullptr);

        // Camera with collision detection
        ctx.followCameraSystem->updateWithCollision(*ctx.registry, *ctx.buildingCuller, nullptr);

        ctx.physicsSystem->update(*ctx.registry, ctx.dt);
        ctx.collisionSystem->update(*ctx.registry);
        ctx.animationSystem->update(*ctx.registry, ctx.dt);
        ctx.skeletonSystem->update(*ctx.registry);

        // Update monster AI
        auto* protagonistT = ctx.registry->getTransform(ctx.protagonist);
        if (ctx.monsterManager && protagonistT) {
            MonsterManager::UpdateResult monsterResult = ctx.monsterManager->update(ctx.dt, protagonistT->position);

            // Check if monster started chasing - trigger death cinematic
            if (monsterResult.chaseStarted) {
                ctx.deathCinematicDistance = monsterResult.distanceToPlayer;
                ctx.sceneManager->switchTo(SceneType::DeathCinematic);
                return;
            }
        }

        // LOD updates
        if (protagonistT && ctx.fingHighDetail && ctx.fingLowDetail) {
            RenderHelpers::updateFingLOD(*ctx.registry, *ctx.gameState, ctx.fingBuilding,
                protagonistT->position, *ctx.fingHighDetail, *ctx.fingLowDetail, ctx.lodSwitchDistance);
        }
    }

    void render(SceneContext& ctx) override {
        auto* cam = ctx.registry->getCamera(ctx.camera);
        auto* camT = ctx.registry->getTransform(ctx.camera);
        auto* protagonistT = ctx.registry->getTransform(ctx.protagonist);
        auto* protagonistFacing = ctx.registry->getFacingDirection(ctx.protagonist);
        auto* ft = ctx.registry->getFollowTarget(ctx.camera);
        auto* fingBuildingT = ctx.registry->getTransform(ctx.fingBuilding);

        glm::mat4 playView = glm::mat4(1.0f);
        glm::mat4 projection = glm::mat4(1.0f);
        glm::vec3 cameraPos = glm::vec3(0.0f);

        if (cam && camT && protagonistT && protagonistFacing && ft) {
            glm::vec3 lookAtPos = FollowCameraSystem::getLookAtPosition(*protagonistT, *ft, protagonistFacing->yaw);
            playView = glm::lookAt(camT->position, lookAtPos, glm::vec3(0.0f, 1.0f, 0.0f));
            projection = cam->projectionMatrix(ctx.aspectRatio);
            cameraPos = camT->position;
        }

        // Update building culling
        ctx.buildingCuller->update(playView, projection, cameraPos, ctx.buildingMaxRenderDistance);

        // === SHADOW PASS ===
        glm::vec3 focusPoint = protagonistT ? protagonistT->position : glm::vec3(0.0f);
        glm::mat4 lightSpaceMatrix = RenderHelpers::computeLightSpaceMatrix(focusPoint, ctx.lightDir);

        ctx.renderPipeline->beginShadowPass();
        ctx.renderPipeline->renderShadowCasters(lightSpaceMatrix, cameraPos);
        ctx.renderPipeline->endShadowPass();

        // === MAIN RENDER PASS ===
        ctx.renderPipeline->beginMainPass(ctx.gameState->toonShadingEnabled);

        // Debug axes
        if (GameConfig::SHOW_AXES && cam && camT && ctx.axes) {
            glm::mat4 vp = projection * playView;
            ctx.colorShader->use();
            ctx.colorShader->setMat4("uMVP", vp);
            ctx.axes->draw();
        }

        // Render scene - reset fog to config values (menu may have changed them)
        RenderHelpers::setupRenderSystem(*ctx.renderSystem, ctx.gameState->fogEnabled, true,
                                          ctx.shadowDepthTexture, lightSpaceMatrix);
        ctx.renderSystem->setFogDensity(GameConfig::FOG_DENSITY);
        ctx.renderSystem->setFogColor(GameConfig::FOG_COLOR);
        ctx.renderSystem->update(*ctx.registry, ctx.aspectRatio);

        // Render buildings
        BuildingRenderParams params;
        params.view = playView;
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
        RenderHelpers::renderGroundPlane(*ctx.groundShader, playView, projection, lightSpaceMatrix,
            ctx.lightDir, cameraPos, ctx.gameState->fogEnabled, true,
            ctx.snowTexture, ctx.shadowDepthTexture, ctx.planeVAO,
            GameConfig::FOG_DENSITY, GameConfig::FOG_COLOR);

        // Render monster danger zones (red circles showing detection radius)
        if (ctx.monsterManager) {
            std::vector<glm::vec3> dangerZonePositions = ctx.monsterManager->getPositions();
            ctx.renderPipeline->renderDangerZones(playView, projection, dangerZonePositions, MonsterData::DETECTION_RADIUS);
        }

        // Render sun
        ctx.renderPipeline->renderSun(playView, projection, cameraPos);

        // Render comets
        ctx.renderPipeline->renderComets(playView, projection, cameraPos);

        // Render 3D snow particles
        if (protagonistT) {
            ctx.renderPipeline->renderSnow(playView, projection, protagonistT->position);
        }

        // Render snow overlay
        RenderHelpers::renderSnowOverlay(*ctx.overlayShader, ctx.overlayVAO, *ctx.gameState);

        // === TOON POST-PROCESSING ===
        if (ctx.gameState->toonShadingEnabled) {
            ctx.renderPipeline->applyToonPostProcess();
        }

        // Render minimap
        std::vector<glm::vec3> minimapMarkers;
        if (fingBuildingT) {
            minimapMarkers.push_back(fingBuildingT->position);
        }

        // Get monster positions for minimap
        std::vector<glm::vec3> monsterPositions;
        if (ctx.monsterManager) {
            monsterPositions = ctx.monsterManager->getPositions();
        }

        ctx.minimapSystem->render(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT,
            protagonistFacing ? protagonistFacing->yaw : 0.0f,
            ctx.uiSystem->fonts(), ctx.uiSystem->textCache(),
            protagonistT ? protagonistT->position : glm::vec3(0.0f),
            minimapMarkers, *ctx.buildingFootprints, monsterPositions);

        // Render UI
        ctx.uiSystem->update(*ctx.registry, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);

        // === FINAL RESOLVE AND BLIT ===
        ctx.renderPipeline->finalResolveAndBlit();

        // === DEBUG: Shadow map visualization ===
        ctx.renderPipeline->renderShadowMapDebug();
    }

    void onExit(SceneContext& ctx) override {
        ctx.registry->getUIText(ctx.sprintHint)->visible = false;
    }
};
