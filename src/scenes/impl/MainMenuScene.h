#pragma once
#include "../IScene.h"
#include "../SceneContext.h"
#include "../SceneManager.h"
#include "../../ecs/Registry.h"
#include "../../ecs/systems/UISystem.h"
#include "../../ecs/systems/RenderSystem.h"
#include "../../core/GameState.h"
#include "../../core/GameConfig.h"
#include "../RenderHelpers.h"
#include "../../rendering/RenderPipeline.h"
#include "../../Shader.h"
#include <glm/gtc/matrix_transform.hpp>

class MainMenuScene : public IScene {
public:
    void onEnter(SceneContext& ctx) override {
        ctx.inputSystem->captureMouse(false);

        // Show menu UI
        ctx.registry->getUIText(ctx.menuOption1)->visible = true;
        ctx.registry->getUIText(ctx.menuOption2)->visible = true;
        ctx.registry->getUIText(ctx.menuOption3)->visible = true;

        // Update colors based on selection
        updateMenuColors(ctx);

        // Reset intro text state when entering main menu
        ctx.gameState->resetIntroText();
    }

    void update(SceneContext& ctx) override {
        // Menu navigation (3 options: 0=PLAY, 1=GOD MODE, 2=EXIT)
        if (ctx.input.upPressed) {
            ctx.gameState->menuSelection = (ctx.gameState->menuSelection + 2) % 3;  // Wrap up
            updateMenuColors(ctx);
        }
        if (ctx.input.downPressed) {
            ctx.gameState->menuSelection = (ctx.gameState->menuSelection + 1) % 3;  // Wrap down
            updateMenuColors(ctx);
        }

        if (ctx.input.enterPressed) {
            if (ctx.gameState->menuSelection == 0) {
                ctx.sceneManager->switchTo(SceneType::IntroText);
            } else if (ctx.gameState->menuSelection == 1) {
                ctx.sceneManager->switchTo(SceneType::GodMode);
            } else {
                // Exit the game
                ctx.gameState->shouldQuit = true;
            }
        }
    }

    void render(SceneContext& ctx) override {
        // Static camera backdrop
        const glm::vec3 menuCamPos = glm::vec3(-4.82f, 4.57f, 19.15f);
        const glm::vec3 menuCamLookAt = glm::vec3(-4.16f, 4.81f, 19.85f);
        glm::mat4 menuView = glm::lookAt(menuCamPos, menuCamLookAt, glm::vec3(0.0f, 1.0f, 0.0f));

        auto* cam = ctx.registry->getCamera(ctx.camera);
        glm::mat4 projection = cam ? cam->projectionMatrix(ctx.aspectRatio) : glm::mat4(1.0f);

        // Clear with sky color (render directly to screen, no MSAA for menu)
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
        glClearColor(0.2f, 0.2f, 0.22f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render FING model with very low fog for menu backdrop
        constexpr float menuFogDensity = 0.002f;  // Much lower than normal game fog
        ctx.renderSystem->setFogEnabled(ctx.gameState->fogEnabled);
        ctx.renderSystem->setFogDensity(menuFogDensity);
        ctx.renderSystem->updateWithView(*ctx.registry, ctx.aspectRatio, menuView);

        // Render ground plane (no buildings, no shadows, low fog)
        RenderHelpers::renderGroundPlane(*ctx.groundShader, menuView, projection, glm::mat4(1.0f),
            ctx.lightDir, menuCamPos, ctx.gameState->fogEnabled, false, ctx.snowTexture, 0, ctx.planeVAO,
            menuFogDensity);

        // Render snow overlay
        RenderHelpers::renderSnowOverlay(*ctx.overlayShader, ctx.overlayVAO, *ctx.gameState);

        // Render falling comets (custom fall direction for menu backdrop)
        glm::vec3 menuCometFallDir = glm::normalize(glm::vec3(0.85f, -0.12f, 0.4f));
        glm::vec3 menuCometColor = glm::vec3(1.0f, 0.4f, 0.1f);
        ctx.renderPipeline->renderComets(menuView, projection, menuCamPos, menuCometFallDir, menuCometColor);

        // Draw 85% black overlay
        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        ctx.solidOverlayShader->use();
        ctx.solidOverlayShader->setVec4("uColor", glm::vec4(0.0f, 0.0f, 0.0f, 0.85f));
        glBindVertexArray(ctx.overlayVAO);
        glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

        glEnable(GL_DEPTH_TEST);

        // Render UI on top
        ctx.uiSystem->update(*ctx.registry, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
    }

    void onExit(SceneContext& ctx) override {
        // Hide menu UI
        ctx.registry->getUIText(ctx.menuOption1)->visible = false;
        ctx.registry->getUIText(ctx.menuOption2)->visible = false;
        ctx.registry->getUIText(ctx.menuOption3)->visible = false;
    }

private:
    const glm::vec4 m_colorSelected = glm::vec4(255.0f, 255.0f, 255.0f, 255.0f);
    const glm::vec4 m_colorUnselected = glm::vec4(128.0f, 128.0f, 128.0f, 255.0f);

    void updateMenuColors(SceneContext& ctx) {
        auto* text1 = ctx.registry->getUIText(ctx.menuOption1);
        auto* text2 = ctx.registry->getUIText(ctx.menuOption2);
        auto* text3 = ctx.registry->getUIText(ctx.menuOption3);
        if (text1 && text2 && text3) {
            text1->color = (ctx.gameState->menuSelection == 0) ? m_colorSelected : m_colorUnselected;
            text2->color = (ctx.gameState->menuSelection == 1) ? m_colorSelected : m_colorUnselected;
            text3->color = (ctx.gameState->menuSelection == 2) ? m_colorSelected : m_colorUnselected;
            ctx.uiSystem->clearCache();
        }
    }
};
