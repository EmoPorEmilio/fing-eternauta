#pragma once
#include "IScene.h"
#include "SceneContext.h"
#include <unordered_map>
#include <memory>

enum class SceneType {
    MainMenu,
    IntroText,       // Typewriter text intro
    IntroCinematic,  // Camera path before gameplay starts
    PlayGame,
    GodMode,
    PauseMenu
};

class SceneManager {
public:
    SceneManager() = default;

    // Register a scene instance for a given type
    void registerScene(SceneType type, std::unique_ptr<IScene> scene) {
        m_scenes[type] = std::move(scene);
    }

    // Get current scene type
    SceneType current() const { return m_currentScene; }

    // Get previous scene type
    SceneType previous() const { return m_previousScene; }

    // Request a scene switch (processed on next update)
    void switchTo(SceneType scene) {
        m_previousScene = m_currentScene;
        m_nextScene = scene;
        m_sceneChangeRequested = true;
    }

    // Check if scene just changed (call once per frame, resets flag)
    bool hasSceneChanged() {
        bool changed = m_sceneJustChanged;
        m_sceneJustChanged = false;
        return changed;
    }

    // Process scene transitions and call lifecycle methods
    // Call this once per frame before scene update/render
    void processTransitions(SceneContext& ctx) {
        if (m_sceneChangeRequested) {
            m_sceneChangeRequested = false;

            // Exit current scene
            if (m_currentScenePtr) {
                m_currentScenePtr->onExit(ctx);
            }

            // Switch to new scene
            m_currentScene = m_nextScene;
            auto it = m_scenes.find(m_currentScene);
            m_currentScenePtr = (it != m_scenes.end()) ? it->second.get() : nullptr;

            // Enter new scene
            if (m_currentScenePtr) {
                m_currentScenePtr->onEnter(ctx);
            }

            m_sceneJustChanged = true;
        }
    }

    // Update current scene
    void update(SceneContext& ctx) {
        if (m_currentScenePtr) {
            m_currentScenePtr->update(ctx);
        }
    }

    // Render current scene
    void render(SceneContext& ctx) {
        if (m_currentScenePtr) {
            m_currentScenePtr->render(ctx);
        }
    }

    // Force initial scene enter (call once after all scenes registered)
    void initialize(SceneContext& ctx) {
        auto it = m_scenes.find(m_currentScene);
        m_currentScenePtr = (it != m_scenes.end()) ? it->second.get() : nullptr;
        if (m_currentScenePtr) {
            m_currentScenePtr->onEnter(ctx);
        }
    }

private:
    std::unordered_map<SceneType, std::unique_ptr<IScene>> m_scenes;
    IScene* m_currentScenePtr = nullptr;

    SceneType m_currentScene = SceneType::MainMenu;
    SceneType m_previousScene = SceneType::MainMenu;
    SceneType m_nextScene = SceneType::MainMenu;

    bool m_sceneChangeRequested = false;
    bool m_sceneJustChanged = false;
};
