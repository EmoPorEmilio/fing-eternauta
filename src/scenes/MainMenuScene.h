#pragma once
#include "Scene.h"
#include "SceneManager.h"

class MainMenuScene : public Scene {
public:
    MainMenuScene(Entity menuOption1, Entity menuOption2)
        : m_menuOption1(menuOption1), m_menuOption2(menuOption2) {}

    void onEnter(SceneContext& ctx) override {
        ctx.inputSystem.captureMouse(false);

        // Show menu options
        auto* text1 = ctx.registry.getUIText(m_menuOption1);
        auto* text2 = ctx.registry.getUIText(m_menuOption2);
        if (text1) text1->visible = true;
        if (text2) text2->visible = true;

        // Reset selection and update colors
        ctx.gameState.menuSelection = 0;
        updateMenuColors(ctx);
    }

    void onExit(SceneContext& ctx) override {
        // Hide menu options
        auto* text1 = ctx.registry.getUIText(m_menuOption1);
        auto* text2 = ctx.registry.getUIText(m_menuOption2);
        if (text1) text1->visible = false;
        if (text2) text2->visible = false;
    }

    void update(SceneContext& ctx, const InputState& input, float dt) override {
        // Menu navigation
        if (input.upPressed || input.downPressed) {
            ctx.gameState.menuSelection = 1 - ctx.gameState.menuSelection;
            updateMenuColors(ctx);
        }

        // Selection
        if (input.enterPressed) {
            if (ctx.gameState.menuSelection == 0) {
                ctx.sceneManager.switchTo(SceneType::IntroText);
            } else {
                ctx.sceneManager.switchTo(SceneType::GodMode);
            }
        }
    }

    void render(SceneContext& ctx) override {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ctx.uiSystem.update(ctx.registry, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
    }

private:
    Entity m_menuOption1;
    Entity m_menuOption2;

    static constexpr glm::vec4 SELECTED_COLOR{255.0f, 255.0f, 255.0f, 255.0f};
    static constexpr glm::vec4 UNSELECTED_COLOR{128.0f, 128.0f, 128.0f, 255.0f};

    void updateMenuColors(SceneContext& ctx) {
        auto* text1 = ctx.registry.getUIText(m_menuOption1);
        auto* text2 = ctx.registry.getUIText(m_menuOption2);
        if (text1 && text2) {
            text1->color = (ctx.gameState.menuSelection == 0) ? SELECTED_COLOR : UNSELECTED_COLOR;
            text2->color = (ctx.gameState.menuSelection == 1) ? SELECTED_COLOR : UNSELECTED_COLOR;
            ctx.uiSystem.clearCache();
        }
    }
};
