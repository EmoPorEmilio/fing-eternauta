#pragma once
#include "Scene.h"
#include "SceneManager.h"
#include <string>

class IntroTextScene : public Scene {
public:
    IntroTextScene(const std::vector<Entity>& textEntities, const std::vector<std::string>& texts)
        : m_textEntities(textEntities), m_texts(texts) {}

    void onEnter(SceneContext& ctx) override {
        ctx.inputSystem.captureMouse(false);

        // Reset typewriter state
        ctx.gameState.resetIntroText();

        // Show all text entities (empty initially)
        for (Entity e : m_textEntities) {
            auto* text = ctx.registry.getUIText(e);
            if (text) {
                text->visible = true;
                text->text = "";
            }
        }
        ctx.uiSystem.clearCache();
    }

    void onExit(SceneContext& ctx) override {
        // Hide all text entities
        for (Entity e : m_textEntities) {
            auto* text = ctx.registry.getUIText(e);
            if (text) text->visible = false;
        }
    }

    void update(SceneContext& ctx, const InputState& input, float dt) override {
        // Skip intro with Enter or Escape
        if (input.enterPressed || input.escapePressed) {
            ctx.sceneManager.switchTo(SceneType::IntroCinematic);
            return;
        }

        // Typewriter effect
        if (!ctx.gameState.introAllComplete) {
            if (ctx.gameState.introLineComplete) {
                // Waiting between lines
                ctx.gameState.introLinePauseTimer += dt;
                if (ctx.gameState.introLinePauseTimer >= GameConfig::TYPEWRITER_LINE_DELAY) {
                    ctx.gameState.introLinePauseTimer = 0.0f;
                    ctx.gameState.introLineComplete = false;
                    ctx.gameState.introCurrentLine++;
                    ctx.gameState.introCurrentChar = 0;
                    if (ctx.gameState.introCurrentLine >= (int)m_texts.size()) {
                        ctx.gameState.introAllComplete = true;
                    }
                }
            } else if (ctx.gameState.introCurrentLine < (int)m_texts.size()) {
                // Typing characters
                ctx.gameState.introTypewriterTimer += dt;
                while (ctx.gameState.introTypewriterTimer >= GameConfig::TYPEWRITER_CHAR_DELAY &&
                       ctx.gameState.introCurrentChar < m_texts[ctx.gameState.introCurrentLine].length()) {
                    ctx.gameState.introTypewriterTimer -= GameConfig::TYPEWRITER_CHAR_DELAY;
                    ctx.gameState.introCurrentChar++;
                    // Update the text entity
                    auto* text = ctx.registry.getUIText(m_textEntities[ctx.gameState.introCurrentLine]);
                    if (text) {
                        text->text = m_texts[ctx.gameState.introCurrentLine].substr(0, ctx.gameState.introCurrentChar);
                        ctx.uiSystem.clearCache();
                    }
                }
                // Check if line is complete
                if (ctx.gameState.introCurrentChar >= m_texts[ctx.gameState.introCurrentLine].length()) {
                    ctx.gameState.introLineComplete = true;
                    ctx.gameState.introLinePauseTimer = 0.0f;
                }
            }
        } else {
            // All text complete, wait then transition
            ctx.gameState.introLinePauseTimer += dt;
            if (ctx.gameState.introLinePauseTimer >= 2.0f) {
                ctx.sceneManager.switchTo(SceneType::IntroCinematic);
            }
        }
    }

    void render(SceneContext& ctx) override {
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        ctx.uiSystem.update(ctx.registry, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
    }

private:
    std::vector<Entity> m_textEntities;
    std::vector<std::string> m_texts;
};
