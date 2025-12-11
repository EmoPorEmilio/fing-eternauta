#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <string>
#include "../ecs/Entity.h"
#include "../ecs/systems/InputSystem.h"
#include "../ecs/components/Mesh.h"

// Forward declarations
class Registry;
struct GameState;
class SceneManager;
class RenderSystem;
class UISystem;
class MinimapSystem;
class CinematicSystem;
class AnimationSystem;
class SkeletonSystem;
class PhysicsSystem;
class CollisionSystem;
class PlayerMovementSystem;
class CameraOrbitSystem;
class FollowCameraSystem;
class FreeCameraSystem;
class BuildingCuller;
class Shader;
class RenderPipeline;
class MonsterManager;
struct AxisRenderer;
struct Mesh;
struct MeshGroup;

// All shared resources passed to scenes
// Scenes should not own these - they're owned by main.cpp
struct SceneContext {
    // Core ECS
    Registry* registry = nullptr;
    GameState* gameState = nullptr;
    SceneManager* sceneManager = nullptr;

    // Input (updated each frame)
    InputSystem* inputSystem = nullptr;
    InputState input;
    float dt = 0.0f;
    float aspectRatio = 16.0f / 9.0f;

    // Systems
    RenderSystem* renderSystem = nullptr;
    UISystem* uiSystem = nullptr;
    MinimapSystem* minimapSystem = nullptr;
    CinematicSystem* cinematicSystem = nullptr;
    AnimationSystem* animationSystem = nullptr;
    SkeletonSystem* skeletonSystem = nullptr;
    PhysicsSystem* physicsSystem = nullptr;
    CollisionSystem* collisionSystem = nullptr;
    PlayerMovementSystem* playerMovementSystem = nullptr;
    CameraOrbitSystem* cameraOrbitSystem = nullptr;
    FollowCameraSystem* followCameraSystem = nullptr;
    FreeCameraSystem* freeCameraSystem = nullptr;

    // Render pipeline
    RenderPipeline* renderPipeline = nullptr;

    // Building culling
    BuildingCuller* buildingCuller = nullptr;
    Mesh* buildingBoxMesh = nullptr;
    Shader* buildingInstancedShader = nullptr;
    Shader* depthInstancedShader = nullptr;
    float buildingMaxRenderDistance = 0.0f;

    // Shaders
    Shader* groundShader = nullptr;
    Shader* colorShader = nullptr;
    Shader* overlayShader = nullptr;
    Shader* solidOverlayShader = nullptr;
    Shader* sunShader = nullptr;
    Shader* cometShader = nullptr;
    Shader* depthShader = nullptr;
    Shader* skinnedDepthShader = nullptr;
    Shader* motionBlurShader = nullptr;
    Shader* toonPostShader = nullptr;
    Shader* blitShader = nullptr;
    Shader* snowShader = nullptr;
    Shader* radialBlurShader = nullptr;

    // Textures
    GLuint snowTexture = 0;
    GLuint brickTexture = 0;
    GLuint brickNormalMap = 0;

    // VAOs
    GLuint planeVAO = 0;
    GLuint overlayVAO = 0;
    GLuint sunVAO = 0;

    // FBOs
    GLuint shadowFBO = 0;
    GLuint shadowDepthTexture = 0;
    GLuint msaaFBO = 0;
    GLuint resolveFBO = 0;
    GLuint resolveColorTex = 0;
    GLuint cinematicMsaaFBO = 0;
    GLuint motionBlurFBO = 0;
    GLuint motionBlurColorTex = 0;
    GLuint motionBlurDepthTex = 0;
    GLuint toonFBO = 0;
    GLuint toonColorTex = 0;

    // Motion blur state
    glm::mat4* prevViewProjection = nullptr;

    // Debug renderer
    AxisRenderer* axes = nullptr;

    // Key entities
    Entity protagonist = NULL_ENTITY;
    Entity camera = NULL_ENTITY;
    Entity fingBuilding = NULL_ENTITY;

    // NPC entities (for dancing near FING)
    std::vector<Entity> npcs;

    // Monster entity (debug - single monster for testing)
    Entity monster = NULL_ENTITY;
    bool monsterFrenzy = false;  // When true, monster speed is 10x

    // Monster system
    MonsterManager* monsterManager = nullptr;

    // FING building data for LOD and collision
    MeshGroup* fingHighDetail = nullptr;
    MeshGroup* fingLowDetail = nullptr;
    glm::vec3 fingWorldCenter;
    glm::vec3 fingWorldHalfExtents;
    float lodSwitchDistance = 0.0f;

    // Comet rendering data
    MeshGroup* cometMeshGroup = nullptr;
    int numComets = 0;
    float cometFallSpeed = 0.0f;
    float cometCycleTime = 0.0f;
    float cometFallDistance = 0.0f;
    float cometScale = 0.0f;
    glm::vec3 cometFallDir;
    glm::vec3 cometColor;

    // Snow particle rendering data
    GLuint snowVAO = 0;
    GLuint snowInstanceVBO = 0;
    int snowParticleCount = 0;

    // Danger zone rendering (monster detection radius)
    Shader* dangerZoneShader = nullptr;
    GLuint dangerZoneVAO = 0;

    // Light direction
    glm::vec3 lightDir;

    // Building footprints for minimap (min XZ, max XZ pairs)
    std::vector<std::pair<glm::vec2, glm::vec2>>* buildingFootprints = nullptr;

    // Menu UI entities
    Entity menuOption1 = NULL_ENTITY;
    Entity menuOption2 = NULL_ENTITY;
    Entity menuOption3 = NULL_ENTITY;  // EXIT option
    Entity sprintHint = NULL_ENTITY;
    Entity godModeHint = NULL_ENTITY;

    // Pause menu UI entities
    Entity pauseFogToggle = NULL_ENTITY;
    Entity pauseSnowToggle = NULL_ENTITY;
    Entity pauseSnowSpeed = NULL_ENTITY;
    Entity pauseSnowAngle = NULL_ENTITY;
    Entity pauseSnowBlur = NULL_ENTITY;
    Entity pauseToonToggle = NULL_ENTITY;
    Entity pauseMenuOption = NULL_ENTITY;

    // Intro text entities and content
    std::vector<Entity>* introTextEntities = nullptr;
    const std::vector<std::string>* introTexts = nullptr;

    // Death screen UI
    Entity youDiedText = NULL_ENTITY;

    // Death cinematic data
    float deathCinematicDistance = 0.0f;  // Distance from monster when chase started
};
