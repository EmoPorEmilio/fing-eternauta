#pragma once
#include "../ecs/Registry.h"
#include "../ecs/systems/InputSystem.h"
#include "../ecs/systems/RenderSystem.h"
#include "../ecs/systems/UISystem.h"
#include "../ecs/systems/AnimationSystem.h"
#include "../ecs/systems/SkeletonSystem.h"
#include "../ecs/systems/PlayerMovementSystem.h"
#include "../ecs/systems/FollowCameraSystem.h"
#include "../ecs/systems/FreeCameraSystem.h"
#include "../ecs/systems/CollisionSystem.h"
#include "../ecs/systems/CinematicSystem.h"
#include "../ecs/systems/MinimapSystem.h"
#include "../core/GameState.h"
#include "../core/GameConfig.h"
#include "../Shader.h"
#include "../procedural/BuildingGenerator.h"
#include <vector>

class SceneManager;

// Shared context passed to all scenes - avoids massive parameter lists
struct SceneContext {
    Registry& registry;
    SceneManager& sceneManager;
    GameState& gameState;
    InputSystem& inputSystem;
    RenderSystem& renderSystem;
    UISystem& uiSystem;
    AnimationSystem& animationSystem;
    SkeletonSystem& skeletonSystem;
    PlayerMovementSystem& playerMovementSystem;
    FollowCameraSystem& followCameraSystem;
    FreeCameraSystem& freeCameraSystem;
    CollisionSystem& collisionSystem;
    CinematicSystem& cinematicSystem;
    MinimapSystem& minimapSystem;

    // Shaders
    Shader& colorShader;
    Shader& groundShader;
    Shader& depthShader;
    Shader& overlayShader;
    Shader& motionBlurShader;

    // Key entities
    Entity protagonist;
    Entity camera;
    Entity fingBuilding;
    Entity ground;

    // Building system
    std::vector<Entity>& buildingEntityPool;
    const std::vector<BuildingGenerator::BuildingData>& buildingDataList;
    const std::vector<BuildingGenerator::BuildingFootprint>& buildingFootprints;
    int buildingRenderRadius;

    // LOD meshes
    MeshGroup& fingHighDetail;
    MeshGroup& fingLowDetail;
    float lodSwitchDistance;

    // GL resources
    GLuint shadowFBO;
    GLuint shadowDepthTexture;
    int shadowWidth;
    int shadowHeight;
    GLuint motionBlurFBO[2];
    GLuint motionBlurColorTex[2];
    GLuint overlayVAO;
    GLuint planeVAO;
    GLuint snowTexture;

    // Rendering state
    float aspectRatio;
    glm::vec3 lightDir;
    float cinematicMotionBlurStrength;

    // Debug
    class DebugRenderer* axes;
};

// Base class for all game scenes
class Scene {
public:
    virtual ~Scene() = default;

    // Called when scene becomes active
    virtual void onEnter(SceneContext& ctx) = 0;

    // Called when scene is deactivated
    virtual void onExit(SceneContext& ctx) = 0;

    // Update logic, return false to signal scene wants to transition
    virtual void update(SceneContext& ctx, const InputState& input, float dt) = 0;

    // Render the scene
    virtual void render(SceneContext& ctx) = 0;
};
