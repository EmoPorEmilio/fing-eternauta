#pragma once
#include "../IScene.h"
#include "../SceneContext.h"
#include "../SceneManager.h"
#include "../../ecs/Registry.h"
#include "../../ecs/systems/UISystem.h"
#include "../../core/GameState.h"
#include "../../core/GameConfig.h"
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <cstdio>

class PauseMenuScene : public IScene {
public:
    void onEnter(SceneContext& ctx) override {
        ctx.inputSystem->captureMouse(false);

        // Reset to first option
        ctx.gameState->pauseMenuSelection = 0;

        // Show pause menu UI
        ctx.registry->getUIText(ctx.pauseFogToggle)->visible = true;
        ctx.registry->getUIText(ctx.pauseSnowToggle)->visible = true;
        ctx.registry->getUIText(ctx.pauseSnowSpeed)->visible = true;
        ctx.registry->getUIText(ctx.pauseSnowAngle)->visible = true;
        ctx.registry->getUIText(ctx.pauseSnowBlur)->visible = true;
        ctx.registry->getUIText(ctx.pauseToonToggle)->visible = true;
        ctx.registry->getUIText(ctx.pauseMenuOption)->visible = true;

        // Set initial colors
        updateMenuColors(ctx);
    }

    void update(SceneContext& ctx) override {
        // Resume game on escape
        if (ctx.input.escapePressed) {
            ctx.sceneManager->switchTo(ctx.sceneManager->previous());
            return;
        }

        // Menu navigation
        if (ctx.input.upPressed) {
            ctx.gameState->pauseMenuSelection = (ctx.gameState->pauseMenuSelection - 1 + MENU_ITEMS) % MENU_ITEMS;
            updateMenuColors(ctx);
        }
        if (ctx.input.downPressed) {
            ctx.gameState->pauseMenuSelection = (ctx.gameState->pauseMenuSelection + 1) % MENU_ITEMS;
            updateMenuColors(ctx);
        }

        // Handle left/right for adjustable values
        if (ctx.input.leftPressed || ctx.input.rightPressed) {
            handleValueAdjustment(ctx);
        }

        // Handle enter for toggles and actions
        if (ctx.input.enterPressed) {
            handleEnter(ctx);
        }
    }

    void render(SceneContext& ctx) override {
        // Clear with black
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Only render UI
        ctx.uiSystem->update(*ctx.registry, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
    }

    void onExit(SceneContext& ctx) override {
        // Hide pause menu UI
        ctx.registry->getUIText(ctx.pauseFogToggle)->visible = false;
        ctx.registry->getUIText(ctx.pauseSnowToggle)->visible = false;
        ctx.registry->getUIText(ctx.pauseSnowSpeed)->visible = false;
        ctx.registry->getUIText(ctx.pauseSnowAngle)->visible = false;
        ctx.registry->getUIText(ctx.pauseSnowBlur)->visible = false;
        ctx.registry->getUIText(ctx.pauseToonToggle)->visible = false;
        ctx.registry->getUIText(ctx.pauseMenuOption)->visible = false;
    }

private:
    static constexpr int MENU_ITEMS = 7;
    const glm::vec4 m_colorSelected = glm::vec4(255.0f, 255.0f, 255.0f, 255.0f);
    const glm::vec4 m_colorUnselected = glm::vec4(128.0f, 128.0f, 128.0f, 255.0f);

    void updateMenuColors(SceneContext& ctx) {
        ctx.registry->getUIText(ctx.pauseFogToggle)->color =
            (ctx.gameState->pauseMenuSelection == 0) ? m_colorSelected : m_colorUnselected;
        ctx.registry->getUIText(ctx.pauseSnowToggle)->color =
            (ctx.gameState->pauseMenuSelection == 1) ? m_colorSelected : m_colorUnselected;
        ctx.registry->getUIText(ctx.pauseSnowSpeed)->color =
            (ctx.gameState->pauseMenuSelection == 2) ? m_colorSelected : m_colorUnselected;
        ctx.registry->getUIText(ctx.pauseSnowAngle)->color =
            (ctx.gameState->pauseMenuSelection == 3) ? m_colorSelected : m_colorUnselected;
        ctx.registry->getUIText(ctx.pauseSnowBlur)->color =
            (ctx.gameState->pauseMenuSelection == 4) ? m_colorSelected : m_colorUnselected;
        ctx.registry->getUIText(ctx.pauseToonToggle)->color =
            (ctx.gameState->pauseMenuSelection == 5) ? m_colorSelected : m_colorUnselected;
        ctx.registry->getUIText(ctx.pauseMenuOption)->color =
            (ctx.gameState->pauseMenuSelection == 6) ? m_colorSelected : m_colorUnselected;
        ctx.uiSystem->clearCache();
    }

    void handleValueAdjustment(SceneContext& ctx) {
        float delta = ctx.input.rightPressed ? 1.0f : -1.0f;

        if (ctx.gameState->pauseMenuSelection == 2) {
            // Adjust snow speed (0.1 to 10.0)
            ctx.gameState->snowSpeed = glm::clamp(ctx.gameState->snowSpeed + delta * 0.5f, 0.1f, 10.0f);
            char buf[48];
            snprintf(buf, sizeof(buf), "SNOW SPEED: %.1f  < >", ctx.gameState->snowSpeed);
            ctx.registry->getUIText(ctx.pauseSnowSpeed)->text = buf;
            ctx.uiSystem->clearCache();
        }
        else if (ctx.gameState->pauseMenuSelection == 3) {
            // Adjust snow angle (-180 to 180)
            ctx.gameState->snowAngle += delta * 10.0f;
            if (ctx.gameState->snowAngle > 180.0f) ctx.gameState->snowAngle -= 360.0f;
            if (ctx.gameState->snowAngle < -180.0f) ctx.gameState->snowAngle += 360.0f;
            char buf[48];
            snprintf(buf, sizeof(buf), "SNOW ANGLE: %.0f  < >", ctx.gameState->snowAngle);
            ctx.registry->getUIText(ctx.pauseSnowAngle)->text = buf;
            ctx.uiSystem->clearCache();
        }
        else if (ctx.gameState->pauseMenuSelection == 4) {
            // Adjust snow motion blur (0.0 to 5.0)
            ctx.gameState->snowMotionBlur = glm::clamp(ctx.gameState->snowMotionBlur + delta * 0.5f, 0.0f, 5.0f);
            char buf[48];
            snprintf(buf, sizeof(buf), "SNOW BLUR: %.1f  < >", ctx.gameState->snowMotionBlur);
            ctx.registry->getUIText(ctx.pauseSnowBlur)->text = buf;
            ctx.uiSystem->clearCache();
        }
    }

    void handleEnter(SceneContext& ctx) {
        if (ctx.gameState->pauseMenuSelection == 0) {
            // Toggle fog
            ctx.gameState->fogEnabled = !ctx.gameState->fogEnabled;
            ctx.registry->getUIText(ctx.pauseFogToggle)->text =
                ctx.gameState->fogEnabled ? "FOG: YES" : "FOG: NO";
            ctx.uiSystem->clearCache();
        }
        else if (ctx.gameState->pauseMenuSelection == 1) {
            // Toggle snow
            ctx.gameState->snowEnabled = !ctx.gameState->snowEnabled;
            ctx.registry->getUIText(ctx.pauseSnowToggle)->text =
                ctx.gameState->snowEnabled ? "SNOW: YES" : "SNOW: NO";
            ctx.uiSystem->clearCache();
        }
        else if (ctx.gameState->pauseMenuSelection == 5) {
            // Toggle toon/comic mode
            ctx.gameState->toonShadingEnabled = !ctx.gameState->toonShadingEnabled;
            ctx.registry->getUIText(ctx.pauseToonToggle)->text =
                ctx.gameState->toonShadingEnabled ? "COMIC MODE: YES" : "COMIC MODE: NO";
            ctx.uiSystem->clearCache();
        }
        else if (ctx.gameState->pauseMenuSelection == 6) {
            // Back to main menu
            ctx.sceneManager->switchTo(SceneType::MainMenu);
        }
    }
};
