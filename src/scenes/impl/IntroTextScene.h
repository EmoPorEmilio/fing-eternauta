#pragma once
#include "../IScene.h"
#include "../SceneContext.h"
#include "../SceneManager.h"
#include "../../ecs/Registry.h"
#include "../../ecs/systems/UISystem.h"
#include "../../core/GameState.h"
#include "../../core/GameConfig.h"
#include <glad/glad.h>

class IntroTextScene : public IScene {
public:
    void onEnter(SceneContext& ctx) override {
        // Reset intro text state
        ctx.gameState->resetIntroText();

        // Make all intro text entities visible (but with empty text initially)
        if (ctx.introTextEntities) {
            for (Entity e : *ctx.introTextEntities) {
                auto* text = ctx.registry->getUIText(e);
                if (text) {
                    text->visible = true;
                    text->text = "";
                }
            }
        }
    }

    void update(SceneContext& ctx) override {
        // Skip intro with Enter or Escape
        if (ctx.input.enterPressed || ctx.input.escapePressed) {
            ctx.sceneManager->switchTo(SceneType::IntroCinematic);
            return;
        }

        if (!ctx.introTexts || !ctx.introTextEntities) return;

        // Typewriter effect update
        if (!ctx.gameState->introAllComplete) {
            if (ctx.gameState->introLineComplete) {
                // Waiting between lines
                ctx.gameState->introLinePauseTimer += ctx.dt;
                if (ctx.gameState->introLinePauseTimer >= GameConfig::TYPEWRITER_LINE_DELAY) {
                    ctx.gameState->introLinePauseTimer = 0.0f;
                    ctx.gameState->introLineComplete = false;
                    ctx.gameState->introCurrentLine++;
                    ctx.gameState->introCurrentChar = 0;
                    if (ctx.gameState->introCurrentLine >= (int)ctx.introTexts->size()) {
                        ctx.gameState->introAllComplete = true;
                    }
                }
            } else if (ctx.gameState->introCurrentLine < (int)ctx.introTexts->size()) {
                // Typing characters
                ctx.gameState->introTypewriterTimer += ctx.dt;
                const std::string& currentLine = (*ctx.introTexts)[ctx.gameState->introCurrentLine];

                while (ctx.gameState->introTypewriterTimer >= GameConfig::TYPEWRITER_CHAR_DELAY &&
                       ctx.gameState->introCurrentChar < currentLine.length()) {
                    ctx.gameState->introTypewriterTimer -= GameConfig::TYPEWRITER_CHAR_DELAY;
                    ctx.gameState->introCurrentChar++;

                    // Update the text entity
                    auto* text = ctx.registry->getUIText((*ctx.introTextEntities)[ctx.gameState->introCurrentLine]);
                    if (text) {
                        text->text = currentLine.substr(0, ctx.gameState->introCurrentChar);
                        ctx.uiSystem->clearCache();
                    }
                }

                // Check if line is complete
                if (ctx.gameState->introCurrentChar >= currentLine.length()) {
                    ctx.gameState->introLineComplete = true;
                    ctx.gameState->introLinePauseTimer = 0.0f;
                }
            }
        } else {
            // All text complete, wait a moment then transition
            ctx.gameState->introLinePauseTimer += ctx.dt;
            if (ctx.gameState->introLinePauseTimer >= 2.0f) {
                ctx.sceneManager->switchTo(SceneType::IntroCinematic);
            }
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
        // Hide intro text entities
        if (ctx.introTextEntities) {
            for (Entity e : *ctx.introTextEntities) {
                auto* text = ctx.registry->getUIText(e);
                if (text) {
                    text->visible = false;
                }
            }
        }
    }
};
