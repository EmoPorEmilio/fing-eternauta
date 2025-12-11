#pragma once

struct SceneContext;

// Base interface for all game scenes
// Scenes handle their own update logic and rendering
class IScene {
public:
    virtual ~IScene() = default;

    // Called when transitioning TO this scene
    virtual void onEnter(SceneContext& ctx) = 0;

    // Called every frame while scene is active
    virtual void update(SceneContext& ctx) = 0;

    // Called every frame for rendering
    virtual void render(SceneContext& ctx) = 0;

    // Called when transitioning AWAY from this scene
    virtual void onExit(SceneContext& ctx) = 0;
};
