#pragma once
#include "../IScene.h"
#include "../SceneContext.h"
#include "../SceneManager.h"
#include "../RenderHelpers.h"
#include "../../ecs/Registry.h"
#include "../../ecs/systems/RenderSystem.h"
#include "../../ecs/systems/UISystem.h"
#include "../../ecs/systems/MinimapSystem.h"
#include "../../ecs/systems/FreeCameraSystem.h"
#include "../../ecs/systems/AnimationSystem.h"
#include "../../ecs/systems/SkeletonSystem.h"
#include "../../core/GameState.h"
#include "../../core/GameConfig.h"
#include "../../culling/BuildingCuller.h"
#include "../../rendering/RenderPipeline.h"
#include "../../Shader.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <cstdio>

class GodModeScene : public IScene {
public:
    void onEnter(SceneContext& ctx) override {
        ctx.inputSystem->captureMouse(true);

        // Show god mode hint
        ctx.registry->getUIText(ctx.godModeHint)->visible = true;

        // Only reset camera position when NOT coming from pause menu
        if (ctx.sceneManager->previous() != SceneType::PauseMenu) {
            auto* ct = ctx.registry->getTransform(ctx.camera);
            if (ct) {
                ct->position = glm::vec3(5.0f, 3.0f, 5.0f);
            }
            ctx.freeCameraSystem->setPosition(glm::vec3(5.0f, 3.0f, 5.0f), -45.0f, -15.0f);
        }
    }

    void update(SceneContext& ctx) override {
        // Open pause menu on escape
        if (ctx.input.escapePressed) {
            ctx.sceneManager->switchTo(SceneType::PauseMenu);
            return;
        }

        // Toggle monster frenzy mode with F key
        if (ctx.input.fPressed) {
            ctx.monsterFrenzy = !ctx.monsterFrenzy;
        }

        // Free camera control
        ctx.freeCameraSystem->update(*ctx.registry, ctx.dt, ctx.input.mouseX, ctx.input.mouseY);

        // Debug: Write camera position when P is pressed
        if (ctx.input.pPressed) {
            auto* camT = ctx.registry->getTransform(ctx.camera);
            if (camT) {
                glm::vec3 pos = camT->position;
                glm::vec3 lookAt = pos + ctx.freeCameraSystem->forward();
                FILE* f = fopen("camera_debug.txt", "a");
                if (f) {
                    fprintf(f, "Camera Position: (%.2f, %.2f, %.2f)\n", pos.x, pos.y, pos.z);
                    fprintf(f, "Camera LookAt:   (%.2f, %.2f, %.2f)\n", lookAt.x, lookAt.y, lookAt.z);
                    fprintf(f, "Yaw: %.2f, Pitch: %.2f\n\n",
                            ctx.freeCameraSystem->yaw(), ctx.freeCameraSystem->pitch());
                    fclose(f);
                }
            }
        }

        // Update animations
        ctx.animationSystem->update(*ctx.registry, ctx.dt);
        ctx.skeletonSystem->update(*ctx.registry);

        // Update monster movement - walk forward
        if (ctx.monster != NULL_ENTITY) {
            auto* monsterT = ctx.registry->getTransform(ctx.monster);
            auto* monsterAnim = ctx.registry->getAnimation(ctx.monster);
            if (monsterT) {
                // Frenzy mode multiplier
                float speedMultiplier = ctx.monsterFrenzy ? 10.0f : 1.0f;

                // Monster is rotated 90 around X, then 180 around Y
                // After these rotations, forward direction in world space is (-X, 0, -Z) or toward origin
                // Calculate forward from rotation quaternion
                glm::vec3 forward = monsterT->rotation * glm::vec3(0.0f, 1.0f, 0.0f);  // Model's Y is forward after X rotation
                forward.y = 0.0f;  // Keep movement on ground plane
                if (glm::length(forward) > 0.001f) {
                    forward = glm::normalize(forward);
                }
                float walkSpeed = 0.5f * speedMultiplier;  // units per second
                monsterT->position += forward * walkSpeed * ctx.dt;

                // Set animation speed multiplier
                if (monsterAnim) {
                    monsterAnim->speedMultiplier = speedMultiplier;
                }
            }
        }

        // LOD update based on camera distance
        auto* camT = ctx.registry->getTransform(ctx.camera);
        if (camT && ctx.fingHighDetail && ctx.fingLowDetail) {
            RenderHelpers::updateFingLOD(*ctx.registry, *ctx.gameState, ctx.fingBuilding,
                camT->position, *ctx.fingHighDetail, *ctx.fingLowDetail, ctx.lodSwitchDistance);
        }
    }

    void render(SceneContext& ctx) override {
        auto* cam = ctx.registry->getCamera(ctx.camera);
        auto* camT = ctx.registry->getTransform(ctx.camera);
        if (!cam || !camT) return;

        glm::mat4 view = ctx.freeCameraSystem->getViewMatrix(camT->position);
        glm::mat4 projection = cam->projectionMatrix(ctx.aspectRatio);
        glm::vec3 cameraPos = camT->position;

        // Update building culling
        ctx.buildingCuller->update(view, projection, cameraPos, ctx.buildingMaxRenderDistance);

        // === SHADOW PASS ===
        glm::mat4 lightSpaceMatrix = RenderHelpers::computeLightSpaceMatrix(cameraPos, ctx.lightDir);

        ctx.renderPipeline->beginShadowPass();
        ctx.renderPipeline->renderShadowCasters(lightSpaceMatrix, cameraPos);
        ctx.renderPipeline->endShadowPass();

        // === MAIN RENDER PASS ===
        ctx.renderPipeline->beginMainPass(ctx.gameState->toonShadingEnabled);

        // Debug axes
        if (GameConfig::SHOW_AXES && ctx.axes) {
            glm::mat4 vp = projection * view;
            ctx.colorShader->use();
            ctx.colorShader->setMat4("uMVP", vp);
            ctx.axes->draw();
        }

        // Render ECS entities with shadows
        RenderHelpers::setupRenderSystem(*ctx.renderSystem, ctx.gameState->fogEnabled, true,
                                          ctx.shadowDepthTexture, lightSpaceMatrix);
        ctx.renderSystem->setFogDensity(GameConfig::FOG_DENSITY);
        ctx.renderSystem->setFogColor(GameConfig::FOG_COLOR);
        ctx.renderSystem->updateWithView(*ctx.registry, ctx.aspectRatio, view);

        // Render ground plane with shadows
        RenderHelpers::renderGroundPlane(*ctx.groundShader, view, projection, lightSpaceMatrix,
            ctx.lightDir, cameraPos, ctx.gameState->fogEnabled, true,
            ctx.snowTexture, ctx.shadowDepthTexture, ctx.planeVAO,
            GameConfig::FOG_DENSITY, GameConfig::FOG_COLOR);

        // Render buildings with shadows
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

        // Render sun
        ctx.renderPipeline->renderSun(view, projection, cameraPos);

        // Render comets
        ctx.renderPipeline->renderComets(view, projection, cameraPos);

        // Render 3D snow particles (centered on protagonist)
        auto* protagonistT = ctx.registry->getTransform(ctx.protagonist);
        if (protagonistT) {
            ctx.renderPipeline->renderSnow(view, projection, protagonistT->position);
        }

        // === TOON POST-PROCESSING ===
        if (ctx.gameState->toonShadingEnabled) {
            ctx.renderPipeline->applyToonPostProcess();
        }

        // Render minimap (simplified, no markers)
        ctx.minimapSystem->render(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);

        // Render UI
        ctx.uiSystem->update(*ctx.registry, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);

        // === FINAL RESOLVE AND BLIT ===
        ctx.renderPipeline->finalResolveAndBlit();
    }

    void onExit(SceneContext& ctx) override {
        ctx.registry->getUIText(ctx.godModeHint)->visible = false;
    }
};
