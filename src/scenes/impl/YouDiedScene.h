#pragma once
#include "../IScene.h"
#include "../SceneContext.h"
#include "../SceneManager.h"
#include "../../ecs/Registry.h"
#include "../../ecs/systems/UISystem.h"
#include "../../core/GameState.h"
#include "../../core/GameConfig.h"
#include "../../Shader.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>

class YouDiedScene : public IScene {
public:
    void onEnter(SceneContext& ctx) override {
        ctx.inputSystem->captureMouse(false);

        // Show the YOU DIED text (we'll create this entity)
        if (ctx.youDiedText != NULL_ENTITY) {
            ctx.registry->getUIText(ctx.youDiedText)->visible = true;
        }
    }

    void update(SceneContext& ctx) override {
        // Any key press returns to main menu
        if (ctx.input.enterPressed || ctx.input.escapePressed ||
            ctx.input.upPressed || ctx.input.downPressed) {
            ctx.sceneManager->switchTo(SceneType::MainMenu);
        }
    }

    void render(SceneContext& ctx) override {
        // Clear with black
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glViewport(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Render UI (the YOU DIED text)
        ctx.uiSystem->update(*ctx.registry, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
    }

    void onExit(SceneContext& ctx) override {
        // Hide the YOU DIED text
        if (ctx.youDiedText != NULL_ENTITY) {
            ctx.registry->getUIText(ctx.youDiedText)->visible = false;
        }
    }
};
