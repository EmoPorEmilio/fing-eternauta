#pragma once

enum class SceneType {
    MainMenu,
    PlayGame,
    GodMode
};

class SceneManager {
public:
    SceneManager() = default;

    SceneType current() const { return m_currentScene; }

    void switchTo(SceneType scene) {
        m_previousScene = m_currentScene;
        m_currentScene = scene;
        m_sceneChanged = true;
    }

    bool hasSceneChanged() {
        bool changed = m_sceneChanged;
        m_sceneChanged = false;
        return changed;
    }

    SceneType previous() const { return m_previousScene; }

private:
    SceneType m_currentScene = SceneType::MainMenu;
    SceneType m_previousScene = SceneType::MainMenu;
    bool m_sceneChanged = true;  // Start true to trigger initial setup
};
