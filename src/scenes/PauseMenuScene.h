#pragma once
#include "Scene.h"
#include "SceneManager.h"
#include <cstdio>

class PauseMenuScene : public Scene {
public:
    struct MenuEntities {
        Entity fogToggle;
        Entity snowToggle;
        Entity snowSpeed;
        Entity snowAngle;
        Entity snowBlur;
        Entity backOption;
    };

    PauseMenuScene(const MenuEntities& entities)
        : m_entities(entities) {}

    void onEnter(SceneContext& ctx) override {
        ctx.inputSystem.captureMouse(false);
        ctx.gameState.pauseMenuSelection = 0;

        // Show all menu items
        ctx.registry.getUIText(m_entities.fogToggle)->visible = true;
        ctx.registry.getUIText(m_entities.snowToggle)->visible = true;
        ctx.registry.getUIText(m_entities.snowSpeed)->visible = true;
        ctx.registry.getUIText(m_entities.snowAngle)->visible = true;
        ctx.registry.getUIText(m_entities.snowBlur)->visible = true;
        ctx.registry.getUIText(m_entities.backOption)->visible = true;

        updateMenuColors(ctx);
    }

    void onExit(SceneContext& ctx) override {
        // Hide all menu items
        ctx.registry.getUIText(m_entities.fogToggle)->visible = false;
        ctx.registry.getUIText(m_entities.snowToggle)->visible = false;
        ctx.registry.getUIText(m_entities.snowSpeed)->visible = false;
        ctx.registry.getUIText(m_entities.snowAngle)->visible = false;
        ctx.registry.getUIText(m_entities.snowBlur)->visible = false;
        ctx.registry.getUIText(m_entities.backOption)->visible = false;
    }

    void update(SceneContext& ctx, const InputState& input, float dt) override {
        // Resume on escape
        if (input.escapePressed) {
            ctx.sceneManager.switchTo(ctx.sceneManager.previous());
            return;
        }

        // Navigation
        if (input.upPressed) {
            ctx.gameState.pauseMenuSelection = (ctx.gameState.pauseMenuSelection - 1 + MENU_ITEMS) % MENU_ITEMS;
            updateMenuColors(ctx);
        }
        if (input.downPressed) {
            ctx.gameState.pauseMenuSelection = (ctx.gameState.pauseMenuSelection + 1) % MENU_ITEMS;
            updateMenuColors(ctx);
        }

        // Left/right for value adjustment
        if (input.leftPressed || input.rightPressed) {
            float delta = input.rightPressed ? 1.0f : -1.0f;
            handleValueAdjustment(ctx, delta);
        }

        // Enter for toggles and back
        if (input.enterPressed) {
            handleEnter(ctx);
        }
    }

    void render(SceneContext& ctx) override {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ctx.uiSystem.update(ctx.registry, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
    }

private:
    MenuEntities m_entities;
    static constexpr int MENU_ITEMS = 6;
    static constexpr glm::vec4 SELECTED_COLOR{255.0f, 255.0f, 255.0f, 255.0f};
    static constexpr glm::vec4 UNSELECTED_COLOR{128.0f, 128.0f, 128.0f, 255.0f};

    void updateMenuColors(SceneContext& ctx) {
        int sel = ctx.gameState.pauseMenuSelection;
        ctx.registry.getUIText(m_entities.fogToggle)->color = (sel == 0) ? SELECTED_COLOR : UNSELECTED_COLOR;
        ctx.registry.getUIText(m_entities.snowToggle)->color = (sel == 1) ? SELECTED_COLOR : UNSELECTED_COLOR;
        ctx.registry.getUIText(m_entities.snowSpeed)->color = (sel == 2) ? SELECTED_COLOR : UNSELECTED_COLOR;
        ctx.registry.getUIText(m_entities.snowAngle)->color = (sel == 3) ? SELECTED_COLOR : UNSELECTED_COLOR;
        ctx.registry.getUIText(m_entities.snowBlur)->color = (sel == 4) ? SELECTED_COLOR : UNSELECTED_COLOR;
        ctx.registry.getUIText(m_entities.backOption)->color = (sel == 5) ? SELECTED_COLOR : UNSELECTED_COLOR;
        ctx.uiSystem.clearCache();
    }

    void handleValueAdjustment(SceneContext& ctx, float delta) {
        char buf[48];
        if (ctx.gameState.pauseMenuSelection == 2) {
            ctx.gameState.snowSpeed = glm::clamp(ctx.gameState.snowSpeed + delta * 0.5f, 0.1f, 10.0f);
            snprintf(buf, sizeof(buf), "SNOW SPEED: %.1f  < >", ctx.gameState.snowSpeed);
            ctx.registry.getUIText(m_entities.snowSpeed)->text = buf;
            ctx.uiSystem.clearCache();
        }
        else if (ctx.gameState.pauseMenuSelection == 3) {
            ctx.gameState.snowAngle = ctx.gameState.snowAngle + delta * 10.0f;
            if (ctx.gameState.snowAngle > 180.0f) ctx.gameState.snowAngle -= 360.0f;
            if (ctx.gameState.snowAngle < -180.0f) ctx.gameState.snowAngle += 360.0f;
            snprintf(buf, sizeof(buf), "SNOW ANGLE: %.0f  < >", ctx.gameState.snowAngle);
            ctx.registry.getUIText(m_entities.snowAngle)->text = buf;
            ctx.uiSystem.clearCache();
        }
        else if (ctx.gameState.pauseMenuSelection == 4) {
            ctx.gameState.snowMotionBlur = glm::clamp(ctx.gameState.snowMotionBlur + delta * 0.5f, 0.0f, 5.0f);
            snprintf(buf, sizeof(buf), "SNOW BLUR: %.1f  < >", ctx.gameState.snowMotionBlur);
            ctx.registry.getUIText(m_entities.snowBlur)->text = buf;
            ctx.uiSystem.clearCache();
        }
    }

    void handleEnter(SceneContext& ctx) {
        if (ctx.gameState.pauseMenuSelection == 0) {
            ctx.gameState.fogEnabled = !ctx.gameState.fogEnabled;
            ctx.registry.getUIText(m_entities.fogToggle)->text = ctx.gameState.fogEnabled ? "FOG: YES" : "FOG: NO";
            ctx.uiSystem.clearCache();
        }
        else if (ctx.gameState.pauseMenuSelection == 1) {
            ctx.gameState.snowEnabled = !ctx.gameState.snowEnabled;
            ctx.registry.getUIText(m_entities.snowToggle)->text = ctx.gameState.snowEnabled ? "SNOW: YES" : "SNOW: NO";
            ctx.uiSystem.clearCache();
        }
        else if (ctx.gameState.pauseMenuSelection == 5) {
            ctx.sceneManager.switchTo(SceneType::MainMenu);
        }
    }
};
