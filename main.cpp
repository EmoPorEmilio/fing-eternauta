// fing-eternauta - ECS-based GLB Model Loader with Skeletal Animation
// Controls: WASD move character, Mouse rotate view, ESC exit

#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stb_image.h>
#include <glm/gtx/quaternion.hpp>
#include <iostream>
#include <vector>
#include <cfloat>

#include "src/ecs/Registry.h"
#include "src/ecs/systems/InputSystem.h"
#include "src/ecs/systems/AnimationSystem.h"
#include "src/ecs/systems/SkeletonSystem.h"
#include "src/ecs/systems/RenderSystem.h"
#include "src/ecs/systems/PhysicsSystem.h"
#include "src/ecs/systems/CollisionSystem.h"
#include "src/ecs/systems/PlayerMovementSystem.h"
#include "src/ecs/systems/CameraOrbitSystem.h"
#include "src/ecs/systems/FollowCameraSystem.h"
#include "src/ecs/systems/FreeCameraSystem.h"
#include "src/ecs/systems/UISystem.h"
#include "src/ecs/systems/MinimapSystem.h"
#include "src/ecs/systems/CinematicSystem.h"
#include "src/assets/AssetLoader.h"
#include "src/DebugRenderer.h"
#include "src/Shader.h"
#include "src/scenes/SceneManager.h"
#include "src/scenes/SceneContext.h"
#include "src/scenes/RenderHelpers.h"
#include "src/scenes/impl/MainMenuScene.h"
#include "src/scenes/impl/IntroTextScene.h"
#include "src/scenes/impl/IntroCinematicScene.h"
#include "src/scenes/impl/PlayGameScene.h"
#include "src/scenes/impl/GodModeScene.h"
#include "src/scenes/impl/PauseMenuScene.h"
#include "src/procedural/BuildingGenerator.h"
#include "src/core/GameConfig.h"
#include "src/core/GameState.h"
#include "src/core/WindowManager.h"
#include "src/culling/BuildingCuller.h"
#include "src/core/AssetManager.h"
#include "src/rendering/RenderPipeline.h"
#include "src/core/ConfigLoader.h"

int main(int argc, char* argv[]) {
    // Load configuration from XML (uses defaults if file not found)
    ConfigLoader::load("config.xml");

    // Initialize window and OpenGL context
    WindowManager windowManager;
    if (!windowManager.init()) {
        return -1;
    }

    // Initialize asset manager (loads textures, shaders, models, FBOs)
    AssetManager assetManager;
    if (!assetManager.init()) {
        std::cerr << "Failed to initialize asset manager" << std::endl;
        return -1;
    }

    // Registry
    Registry registry;

    // Scene Manager
    SceneManager sceneManager;

    // Systems
    InputSystem inputSystem;
    PlayerMovementSystem playerMovementSystem;
    CameraOrbitSystem cameraOrbitSystem;
    FollowCameraSystem followCameraSystem;
    FreeCameraSystem freeCameraSystem;
    AnimationSystem animationSystem;
    SkeletonSystem skeletonSystem;
    PhysicsSystem physicsSystem;
    CollisionSystem collisionSystem;
    RenderSystem renderSystem;
    renderSystem.loadShaders();

    UISystem uiSystem;
    if (!uiSystem.init()) {
        std::cerr << "Failed to initialize UI system" << std::endl;
    }

    MinimapSystem minimapSystem;
    minimapSystem.init();

    CinematicSystem cinematicSystem;

    // Load UI fonts
    if (!uiSystem.fonts().loadFont("oxanium", "assets/fonts/Oxanium.ttf", 28)) {
        std::cerr << "Failed to load Oxanium font" << std::endl;
    }
    if (!uiSystem.fonts().loadFont("oxanium_large", "assets/fonts/Oxanium.ttf", 48)) {
        std::cerr << "Failed to load Oxanium large font" << std::endl;
    }
    if (!uiSystem.fonts().loadFont("oxanium_small", "assets/fonts/Oxanium.ttf", 17)) {
        std::cerr << "Failed to load Oxanium small font" << std::endl;
    }
    // 1942 font - load many sizes for testing
    uiSystem.fonts().loadFont("1942_12", "assets/fonts/1942.ttf", 12);
    uiSystem.fonts().loadFont("1942_14", "assets/fonts/1942.ttf", 14);
    uiSystem.fonts().loadFont("1942_16", "assets/fonts/1942.ttf", 16);
    uiSystem.fonts().loadFont("1942_18", "assets/fonts/1942.ttf", 18);
    uiSystem.fonts().loadFont("1942_20", "assets/fonts/1942.ttf", 20);
    uiSystem.fonts().loadFont("1942_22", "assets/fonts/1942.ttf", 22);
    uiSystem.fonts().loadFont("1942_24", "assets/fonts/1942.ttf", 24);
    uiSystem.fonts().loadFont("1942_28", "assets/fonts/1942.ttf", 28);
    uiSystem.fonts().loadFont("1942_32", "assets/fonts/1942.ttf", 32);
    uiSystem.fonts().loadFont("1942_36", "assets/fonts/1942.ttf", 36);
    uiSystem.fonts().loadFont("1942_48", "assets/fonts/1942.ttf", 48);
    // Oxanium extra sizes
    uiSystem.fonts().loadFont("oxanium_12", "assets/fonts/Oxanium.ttf", 12);
    uiSystem.fonts().loadFont("oxanium_14", "assets/fonts/Oxanium.ttf", 14);
    uiSystem.fonts().loadFont("oxanium_16", "assets/fonts/Oxanium.ttf", 16);
    uiSystem.fonts().loadFont("oxanium_18", "assets/fonts/Oxanium.ttf", 18);
    uiSystem.fonts().loadFont("oxanium_20", "assets/fonts/Oxanium.ttf", 20);
    uiSystem.fonts().loadFont("oxanium_22", "assets/fonts/Oxanium.ttf", 22);
    uiSystem.fonts().loadFont("oxanium_24", "assets/fonts/Oxanium.ttf", 24);
    uiSystem.fonts().loadFont("oxanium_32", "assets/fonts/Oxanium.ttf", 32);

    inputSystem.setWindow(windowManager.window());

    // Get loaded assets from AssetManager
    LoadedModel& protagonistData = assetManager.getModel("protagonist");

    // Create protagonist entity
    Entity protagonist = registry.create();
    Transform protagonistTransform;
    protagonistTransform.position = GameConfig::INTRO_CHARACTER_POS;
    protagonistTransform.scale = glm::vec3(GameConfig::PLAYER_SCALE);
    registry.addTransform(protagonist, protagonistTransform);
    registry.addMeshGroup(protagonist, std::move(protagonistData.meshGroup));
    Renderable protagonistRenderable;
    protagonistRenderable.shader = ShaderType::Skinned;
    protagonistRenderable.meshOffset = glm::vec3(0.0f, -25.0f, 0.0f);  // Lower mesh so feet touch ground
    registry.addRenderable(protagonist, protagonistRenderable);

    // Add player controller
    PlayerController playerController;
    playerController.moveSpeed = GameConfig::PLAYER_MOVE_SPEED;
    playerController.turnSpeed = GameConfig::PLAYER_TURN_SPEED;
    registry.addPlayerController(protagonist, playerController);

    // Add facing direction (decoupled from camera)
    FacingDirection facingDir;
    facingDir.yaw = 0.0f;
    facingDir.turnSpeed = GameConfig::PLAYER_TURN_SPEED;
    registry.addFacingDirection(protagonist, facingDir);

    if (protagonistData.skeleton) {
        registry.addSkeleton(protagonist, std::move(*protagonistData.skeleton));

        // Add animation component with clips (clips now live in component)
        Animation anim;
        anim.clipIndex = 0;
        anim.playing = false;
        anim.clips = std::move(protagonistData.clips);  // Clips stored in component
        registry.addAnimation(protagonist, anim);
    }

    // Get FING building models - high detail and LOD versions
    LoadedModel& fingBuildingData = assetManager.getModel("fingHighDetail");
    LoadedModel& fingBuildingLodData = assetManager.getModel("fingLowDetail");

    // Reference mesh groups for LOD switching (owned by AssetManager)
    MeshGroup& fingHighDetail = fingBuildingData.meshGroup;
    MeshGroup& fingLowDetail = fingBuildingLodData.meshGroup;

    // Store model-space bounds for AABB calculation
    ModelBounds fingModelBounds = fingBuildingData.bounds;

    Entity fingBuilding = registry.create();
    Transform fingTransform;
    // Move fing building outside the procedural grid (grid spans roughly -56 to +56)
    fingTransform.position = GameConfig::FING_BUILDING_POS;  // Outside grid, raised high
    fingTransform.rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));  // Rotate to stand upright
    fingTransform.scale = glm::vec3(2.5f);  // 2x larger (was 1.25)
    registry.addTransform(fingBuilding, fingTransform);
    registry.addMeshGroup(fingBuilding, MeshGroup{fingLowDetail.meshes});  // Start with LOD (far away)
    Renderable fingRenderable;
    fingRenderable.shader = ShaderType::Model;  // Non-animated model
    registry.addRenderable(fingBuilding, fingRenderable);

    // Compute world-space AABB for FING building (apply rotation and scale)
    // Model is rotated -90 around X (Y and Z swap), then scaled
    glm::mat4 fingRotMat = glm::toMat4(fingTransform.rotation);
    glm::vec3 fingWorldMin(FLT_MAX), fingWorldMax(-FLT_MAX);
    // Transform all 8 corners of the model-space AABB
    for (int i = 0; i < 8; ++i) {
        glm::vec3 corner(
            (i & 1) ? fingModelBounds.max.x : fingModelBounds.min.x,
            (i & 2) ? fingModelBounds.max.y : fingModelBounds.min.y,
            (i & 4) ? fingModelBounds.max.z : fingModelBounds.min.z
        );
        // Apply rotation, then scale, then translation
        glm::vec3 rotated = glm::vec3(fingRotMat * glm::vec4(corner, 1.0f));
        glm::vec3 scaled = rotated * fingTransform.scale;
        glm::vec3 world = scaled + fingTransform.position;
        fingWorldMin = glm::min(fingWorldMin, world);
        fingWorldMax = glm::max(fingWorldMax, world);
    }
    // Store computed world AABB for collision and exclusion
    glm::vec3 fingWorldHalfExtents = (fingWorldMax - fingWorldMin) * 0.5f;
    glm::vec3 fingWorldCenter = (fingWorldMin + fingWorldMax) * 0.5f;
    std::cout << "FING world AABB: min(" << fingWorldMin.x << ", " << fingWorldMin.y << ", " << fingWorldMin.z << ")"
              << " max(" << fingWorldMax.x << ", " << fingWorldMax.y << ", " << fingWorldMax.z << ")"
              << " halfExtents(" << fingWorldHalfExtents.x << ", " << fingWorldHalfExtents.y << ", " << fingWorldHalfExtents.z << ")" << std::endl;

    // LOD settings
    const float lodSwitchDistance = GameConfig::LOD_SWITCH_DISTANCE;

    // Get comet model for sky effect
    LoadedModel& cometData = assetManager.getModel("comet");
    MeshGroup& cometMeshGroup = cometData.meshGroup;

    // Game state - all runtime variables in one place
    GameState gameState;

    // Get textures from AssetManager
    GLuint brickTexture = assetManager.getTexture("brick");
    GLuint brickNormalMap = assetManager.getTexture("brickNormal");
    GLuint snowTexture = assetManager.getTexture("snow");

    // Generate procedural buildings data (100x100 grid = 10,000 buildings)
    // Exclude buildings that would overlap with FING building AABB (computed from model)
    const float FING_PADDING = 5.0f;  // Extra padding around FING building
    glm::vec2 fingExclusionMin(
        fingWorldMin.x - FING_PADDING,
        fingWorldMin.z - FING_PADDING
    );
    glm::vec2 fingExclusionMax(
        fingWorldMax.x + FING_PADDING,
        fingWorldMax.z + FING_PADDING
    );
    std::vector<BuildingGenerator::BuildingData> buildingDataList = BuildingGenerator::generateBuildingGrid(
        12345, fingExclusionMin, fingExclusionMax);
    Mesh buildingBoxMesh = BuildingGenerator::createUnitBoxMesh();  // Shared mesh for all buildings
    buildingBoxMesh.texture = brickTexture;  // Apply brick texture to buildings
    buildingBoxMesh.normalMap = brickNormalMap;  // Apply brick normal map
    std::cout << "Generated building data for " << buildingDataList.size() << " buildings" << std::endl;

    // Building visibility parameters from config
    const float BUILDING_MAX_RENDER_DISTANCE = GameConfig::BUILDING_RENDER_DISTANCE;
    const int MAX_VISIBLE_BUILDINGS = GameConfig::MAX_VISIBLE_BUILDINGS;

    // Get building footprints for minimap
    auto buildingFootprints = BuildingGenerator::getBuildingFootprints(buildingDataList);

    // Create ground plane entity using AssetManager's planeVAO
    const float planeSize = GameConfig::GROUND_SIZE;

    Entity ground = registry.create();
    Transform groundTransform;
    groundTransform.position = glm::vec3(0.0f, 0.0f, 0.0f);
    registry.addTransform(ground, groundTransform);

    // Create ground mesh using AssetManager's plane VAO
    GLuint planeVAO = assetManager.primitiveVAOs().planeVAO;
    Mesh planeMesh;
    planeMesh.vao = planeVAO;
    planeMesh.indexCount = 6;
    planeMesh.indexType = GL_UNSIGNED_SHORT;
    planeMesh.hasSkinning = false;
    planeMesh.texture = snowTexture;

    MeshGroup groundMeshGroup;
    groundMeshGroup.meshes.push_back(planeMesh);
    registry.addMeshGroup(ground, std::move(groundMeshGroup));

    // Ground collider (large flat box)
    BoxCollider groundCollider;
    groundCollider.halfExtents = glm::vec3(planeSize, 0.1f, planeSize);
    groundCollider.offset = glm::vec3(0.0f, -0.1f, 0.0f);
    registry.addBoxCollider(ground, groundCollider);

    // Create camera with follow target
    Entity camera = registry.create();
    Transform camTransform;
    camTransform.position = glm::vec3(0.0f, 3.0f, 5.0f);
    registry.addTransform(camera, camTransform);
    CameraComponent camComponent;
    camComponent.fov = GameConfig::CAMERA_FOV;
    camComponent.nearPlane = GameConfig::CAMERA_NEAR;
    camComponent.farPlane = GameConfig::CAMERA_FAR;
    camComponent.active = true;
    registry.addCamera(camera, camComponent);

    // Follow target links camera to protagonist (over-shoulder view)
    FollowTarget followTarget;
    followTarget.target = protagonist;
    registry.addFollowTarget(camera, followTarget);

    // Set up intro cinematic camera path
    // Path: Start in front of character, sweep around to behind (follow camera position)
    // Character faces FING building - yaw ~225 degrees
    const float characterYaw = GameConfig::INTRO_CHARACTER_YAW;
    const glm::vec3 characterPos = GameConfig::INTRO_CHARACTER_POS;

    // Use the same calculation as FollowCameraSystem to ensure exact match
    glm::vec3 followCamEndPos = FollowCameraSystem::getCameraPosition(characterPos, followTarget, characterYaw);

    // Calculate look-at point
    float yawRad = glm::radians(characterYaw);
    glm::vec3 forward(-sin(yawRad), 0.0f, -cos(yawRad));
    glm::vec3 followCamLookAt = characterPos + forward * followTarget.lookAhead;
    followCamLookAt.y += 1.0f;  // Eye level

    {
        NurbsCurve introPath;
        // Control points for a smooth arc - start in front of character (facing them), end behind
        // Character faces toward (+X, +Z) diagonal, so "in front" is in that direction
        // All positions are relative to character position
        glm::vec3 camStart = characterPos + glm::vec3(6.0f, 3.0f, 6.0f);  // Start: in front of character (toward FING direction)
        introPath.addControlPoint(camStart);
        introPath.addControlPoint(characterPos + glm::vec3(3.0f, 2.5f, 0.0f));    // Sweep to the side
        introPath.addControlPoint(characterPos + glm::vec3(-2.0f, 2.0f, -2.0f));  // Moving toward final position
        // Add intermediate point close to final to ensure smooth approach (avoid spline snap)
        glm::vec3 nearFinal = glm::mix(characterPos + glm::vec3(-2.0f, 2.0f, -2.0f), followCamEndPos, 0.7f);
        introPath.addControlPoint(nearFinal);
        introPath.addControlPoint(followCamEndPos);                 // End: behind character (follow camera)

        cinematicSystem.setCameraPath(introPath);
        cinematicSystem.setLookAtTarget(protagonist);
        cinematicSystem.setFinalLookAt(followCamLookAt);  // Blend to gameplay look-at at end
        cinematicSystem.setDuration(GameConfig::CINEMATIC_DURATION);

        // Character stays facing toward FING the whole time
        cinematicSystem.setCharacterEntity(protagonist);
        cinematicSystem.setCharacterYaw(characterYaw, characterYaw);
    }

    // Menu UI entities
    Entity menuOption1 = registry.create();
    UIText menuText1;
    menuText1.text = "PLAY GAME";
    menuText1.fontId = "oxanium_large";
    menuText1.fontSize = 48;
    menuText1.anchor = AnchorPoint::Center;
    menuText1.offset = glm::vec2(0.0f, -30.0f);
    menuText1.horizontalAlign = HorizontalAlign::Center;
    menuText1.color = glm::vec4(255.0f, 255.0f, 255.0f, 255.0f);
    menuText1.visible = false;
    registry.addUIText(menuOption1, menuText1);

    Entity menuOption2 = registry.create();
    UIText menuText2;
    menuText2.text = "GOD MODE";
    menuText2.fontId = "oxanium_large";
    menuText2.fontSize = 48;
    menuText2.anchor = AnchorPoint::Center;
    menuText2.offset = glm::vec2(0.0f, 30.0f);
    menuText2.horizontalAlign = HorizontalAlign::Center;
    menuText2.color = glm::vec4(128.0f, 128.0f, 128.0f, 255.0f);
    menuText2.visible = false;
    registry.addUIText(menuOption2, menuText2);

    // In-game UI
    Entity sprintHint = registry.create();
    UIText sprintText;
    sprintText.text = "PRESS SHIFT TO SPRINT";
    sprintText.fontId = "oxanium";
    sprintText.fontSize = 28;
    sprintText.anchor = AnchorPoint::BottomCenter;
    sprintText.offset = glm::vec2(0.0f, 40.0f);
    sprintText.horizontalAlign = HorizontalAlign::Center;
    sprintText.color = glm::vec4(255.0f, 255.0f, 255.0f, 255.0f);
    sprintText.visible = false;
    registry.addUIText(sprintHint, sprintText);

    // God mode hint
    Entity godModeHint = registry.create();
    UIText godModeText;
    godModeText.text = "GOD MODE - WASD + MOUSE TO FLY";
    godModeText.fontId = "oxanium";
    godModeText.fontSize = 28;
    godModeText.anchor = AnchorPoint::BottomCenter;
    godModeText.offset = glm::vec2(0.0f, 40.0f);
    godModeText.horizontalAlign = HorizontalAlign::Center;
    godModeText.color = glm::vec4(255.0f, 255.0f, 255.0f, 255.0f);
    godModeText.visible = false;
    registry.addUIText(godModeHint, godModeText);

    // Pause menu UI
    Entity pauseFogToggle = registry.create();
    UIText fogToggleText;
    fogToggleText.text = "FOG: YES";  // Match default fogEnabled = true
    fogToggleText.fontId = "oxanium_large";
    fogToggleText.fontSize = 48;
    fogToggleText.anchor = AnchorPoint::Center;
    fogToggleText.offset = glm::vec2(0.0f, -90.0f);
    fogToggleText.horizontalAlign = HorizontalAlign::Center;
    fogToggleText.color = glm::vec4(255.0f, 255.0f, 255.0f, 255.0f);
    fogToggleText.visible = false;
    registry.addUIText(pauseFogToggle, fogToggleText);

    Entity pauseSnowToggle = registry.create();
    UIText snowToggleText;
    snowToggleText.text = "SNOW: YES";
    snowToggleText.fontId = "oxanium_large";
    snowToggleText.fontSize = 48;
    snowToggleText.anchor = AnchorPoint::Center;
    snowToggleText.offset = glm::vec2(0.0f, -30.0f);
    snowToggleText.horizontalAlign = HorizontalAlign::Center;
    snowToggleText.color = glm::vec4(128.0f, 128.0f, 128.0f, 255.0f);
    snowToggleText.visible = false;
    registry.addUIText(pauseSnowToggle, snowToggleText);

    Entity pauseSnowSpeed = registry.create();
    UIText snowSpeedText;
    snowSpeedText.text = "SNOW SPEED: 5.0  < >";
    snowSpeedText.fontId = "oxanium_large";
    snowSpeedText.fontSize = 48;
    snowSpeedText.anchor = AnchorPoint::Center;
    snowSpeedText.offset = glm::vec2(0.0f, 30.0f);
    snowSpeedText.horizontalAlign = HorizontalAlign::Center;
    snowSpeedText.color = glm::vec4(128.0f, 128.0f, 128.0f, 255.0f);
    snowSpeedText.visible = false;
    registry.addUIText(pauseSnowSpeed, snowSpeedText);

    Entity pauseSnowAngle = registry.create();
    UIText snowAngleText;
    snowAngleText.text = "SNOW ANGLE: 40  < >";
    snowAngleText.fontId = "oxanium_large";
    snowAngleText.fontSize = 48;
    snowAngleText.anchor = AnchorPoint::Center;
    snowAngleText.offset = glm::vec2(0.0f, 90.0f);
    snowAngleText.horizontalAlign = HorizontalAlign::Center;
    snowAngleText.color = glm::vec4(128.0f, 128.0f, 128.0f, 255.0f);
    snowAngleText.visible = false;
    registry.addUIText(pauseSnowAngle, snowAngleText);

    Entity pauseSnowBlur = registry.create();
    UIText snowBlurText;
    snowBlurText.text = "SNOW BLUR: 0.0  < >";
    snowBlurText.fontId = "oxanium_large";
    snowBlurText.fontSize = 48;
    snowBlurText.anchor = AnchorPoint::Center;
    snowBlurText.offset = glm::vec2(0.0f, 150.0f);
    snowBlurText.horizontalAlign = HorizontalAlign::Center;
    snowBlurText.color = glm::vec4(128.0f, 128.0f, 128.0f, 255.0f);
    snowBlurText.visible = false;
    registry.addUIText(pauseSnowBlur, snowBlurText);

    Entity pauseToonToggle = registry.create();
    UIText toonToggleText;
    toonToggleText.text = "COMIC MODE: NO";
    toonToggleText.fontId = "oxanium_large";
    toonToggleText.fontSize = 48;
    toonToggleText.anchor = AnchorPoint::Center;
    toonToggleText.offset = glm::vec2(0.0f, 210.0f);
    toonToggleText.horizontalAlign = HorizontalAlign::Center;
    toonToggleText.color = glm::vec4(128.0f, 128.0f, 128.0f, 255.0f);
    toonToggleText.visible = false;
    registry.addUIText(pauseToonToggle, toonToggleText);

    Entity pauseMenuOption = registry.create();
    UIText pauseMenuText;
    pauseMenuText.text = "BACK TO MAIN MENU";
    pauseMenuText.fontId = "oxanium_large";
    pauseMenuText.fontSize = 48;
    pauseMenuText.anchor = AnchorPoint::Center;
    pauseMenuText.offset = glm::vec2(0.0f, 270.0f);
    pauseMenuText.horizontalAlign = HorizontalAlign::Center;
    pauseMenuText.color = glm::vec4(128.0f, 128.0f, 128.0f, 255.0f);
    pauseMenuText.visible = false;
    registry.addUIText(pauseMenuOption, pauseMenuText);

    // Intro text scene - 1942 font with typewriter effect
    std::vector<Entity> introTextEntities;
    const glm::vec4 introTextColor(255.0f, 255.0f, 255.0f, 255.0f);  // White

    // Header: left-aligned but positioned on the right side (doesn't shift during typewriter)
    Entity introHeader = registry.create();
    UIText introHeaderText;
    introHeaderText.text = "";
    introHeaderText.fontId = "1942_32";
    introHeaderText.fontSize = 32;
    introHeaderText.anchor = AnchorPoint::TopLeft;
    introHeaderText.offset = glm::vec2(GameConfig::INTRO_HEADER_X, GameConfig::INTRO_HEADER_Y);
    introHeaderText.horizontalAlign = HorizontalAlign::Left;
    introHeaderText.color = introTextColor;
    introHeaderText.visible = false;
    registry.addUIText(introHeader, introHeaderText);
    introTextEntities.push_back(introHeader);

    // Body paragraphs: left-aligned story text, centered on screen
    const float introLeftMargin = GameConfig::INTRO_BODY_LEFT_MARGIN;
    const float introLineHeight = GameConfig::INTRO_LINE_HEIGHT;
    const float introStartY = GameConfig::INTRO_BODY_START_Y;

    Entity introBody1 = registry.create();
    UIText introBody1Text;
    introBody1Text.text = "";
    introBody1Text.fontId = "1942_48";
    introBody1Text.fontSize = 48;
    introBody1Text.anchor = AnchorPoint::TopLeft;
    introBody1Text.offset = glm::vec2(introLeftMargin, introStartY);
    introBody1Text.horizontalAlign = HorizontalAlign::Left;
    introBody1Text.color = introTextColor;
    introBody1Text.visible = false;
    registry.addUIText(introBody1, introBody1Text);
    introTextEntities.push_back(introBody1);

    Entity introBody2 = registry.create();
    UIText introBody2Text;
    introBody2Text.text = "";
    introBody2Text.fontId = "1942_48";
    introBody2Text.fontSize = 48;
    introBody2Text.anchor = AnchorPoint::TopLeft;
    introBody2Text.offset = glm::vec2(introLeftMargin, introStartY + introLineHeight);
    introBody2Text.horizontalAlign = HorizontalAlign::Left;
    introBody2Text.color = introTextColor;
    introBody2Text.visible = false;
    registry.addUIText(introBody2, introBody2Text);
    introTextEntities.push_back(introBody2);

    Entity introBody3 = registry.create();
    UIText introBody3Text;
    introBody3Text.text = "";
    introBody3Text.fontId = "1942_48";
    introBody3Text.fontSize = 48;
    introBody3Text.anchor = AnchorPoint::TopLeft;
    introBody3Text.offset = glm::vec2(introLeftMargin, introStartY + introLineHeight * 2);
    introBody3Text.horizontalAlign = HorizontalAlign::Left;
    introBody3Text.color = introTextColor;
    introBody3Text.visible = false;
    registry.addUIText(introBody3, introBody3Text);
    introTextEntities.push_back(introBody3);

    Entity introBody4 = registry.create();
    UIText introBody4Text;
    introBody4Text.text = "";
    introBody4Text.fontId = "1942_48";
    introBody4Text.fontSize = 48;
    introBody4Text.anchor = AnchorPoint::TopLeft;
    introBody4Text.offset = glm::vec2(introLeftMargin, introStartY + introLineHeight * 3);
    introBody4Text.horizontalAlign = HorizontalAlign::Left;
    introBody4Text.color = introTextColor;
    introBody4Text.visible = false;
    registry.addUIText(introBody4, introBody4Text);
    introTextEntities.push_back(introBody4);

    Entity introBody5 = registry.create();
    UIText introBody5Text;
    introBody5Text.text = "";
    introBody5Text.fontId = "1942_48";
    introBody5Text.fontSize = 48;
    introBody5Text.anchor = AnchorPoint::TopLeft;
    introBody5Text.offset = glm::vec2(introLeftMargin, introStartY + introLineHeight * 4);
    introBody5Text.horizontalAlign = HorizontalAlign::Left;
    introBody5Text.color = introTextColor;
    introBody5Text.visible = false;
    registry.addUIText(introBody5, introBody5Text);
    introTextEntities.push_back(introBody5);

    // Intro text content for typewriter effect
    const std::vector<std::string> introTexts = {
        "Montevideo, Uruguay, 2025",
        "Seven days have passed since the deadly",
        "snow started falling.",
        "Find us at FING.",
        "Hurry.",
        "They are coming."
    };
    const float introCharDelay = GameConfig::TYPEWRITER_CHAR_DELAY;
    const float introLineDelay = GameConfig::TYPEWRITER_LINE_DELAY;

    // Get shaders from AssetManager
    Shader* groundShader = assetManager.getShader(AssetShader::Ground);
    Shader* buildingInstancedShader = assetManager.getShader(AssetShader::BuildingInstanced);
    Shader* depthInstancedShader = assetManager.getShader(AssetShader::DepthInstanced);
    Shader* colorShader = assetManager.getShader(AssetShader::Color);
    Shader* sunShader = assetManager.getShader(AssetShader::Sun);
    Shader* cometShader = assetManager.getShader(AssetShader::Comet);
    Shader* depthShader = assetManager.getShader(AssetShader::Depth);
    Shader* skinnedDepthShader = assetManager.getShader(AssetShader::SkinnedDepth);
    Shader* motionBlurShader = assetManager.getShader(AssetShader::MotionBlur);
    Shader* toonPostShader = assetManager.getShader(AssetShader::ToonPost);
    Shader* blitShader = assetManager.getShader(AssetShader::Blit);
    Shader* overlayShader = assetManager.getShader(AssetShader::Overlay);
    Shader* solidOverlayShader = assetManager.getShader(AssetShader::SolidOverlay);

    // Get VAOs from AssetManager
    GLuint sunVAO = assetManager.primitiveVAOs().sunVAO;
    GLuint overlayVAO = assetManager.primitiveVAOs().overlayVAO;

    // Get render targets from AssetManager
    const RenderTargets& renderTargets = assetManager.renderTargets();
    GLuint shadowFBO = renderTargets.shadowFBO;
    GLuint shadowDepthTexture = renderTargets.shadowDepthTexture;
    GLuint msaaFBO = renderTargets.msaaFBO;
    GLuint resolveFBO = renderTargets.resolveFBO;
    GLuint resolveColorTex = renderTargets.resolveColorTex;
    GLuint cinematicMsaaFBO = renderTargets.cinematicMsaaFBO;
    GLuint motionBlurFBO = renderTargets.motionBlurFBO;
    GLuint motionBlurColorTex = renderTargets.motionBlurColorTex;
    GLuint motionBlurDepthTex = renderTargets.motionBlurDepthTex;
    GLuint toonFBO = renderTargets.toonFBO;
    GLuint toonColorTex = renderTargets.toonColorTex;

    // Building culling system (octree + frustum + instanced rendering)
    BuildingCuller buildingCuller;
    buildingCuller.init(buildingDataList, MAX_VISIBLE_BUILDINGS);

    // Debug axes
    AxisRenderer axes;
    axes.init();

    // Setup comet instances - spread across the distant sky
    const int NUM_COMETS = 20;
    const float COMET_SKY_DISTANCE = 800.0f;  // Very far in the sky
    const float COMET_SKY_HEIGHT = 400.0f;    // High up
    const float COMET_SKY_SPREAD = 600.0f;    // Horizontal spread
    const float COMET_FALL_DISTANCE = 500.0f;  // How far they fall diagonally
    const float COMET_CYCLE_TIME = 20.0f;  // Seconds for one fall
    const float COMET_FALL_SPEED = 12.5f;  // Speed multiplier
    const float COMET_SCALE = 8.0f;        // Size of comet model (bigger since far away)
    const glm::vec3 COMET_FALL_DIR = glm::normalize(glm::vec3(0.4f, -0.6f, 0.4f));  // Diagonal fall toward +X,+Z
    const glm::vec3 COMET_COLOR = glm::vec3(1.0f, 0.6f, 0.2f);  // Fiery orange

    // Generate start positions for comets in the sky TOWARD FING building (+X, +Z direction)
    // This places them as a backdrop behind FING when viewed from player spawn
    std::vector<glm::vec4> cometInstances(NUM_COMETS);
    {
        srand(42);
        // Direction toward FING from origin (normalized)
        glm::vec3 towardFing = glm::normalize(glm::vec3(1.0f, 0.0f, 1.0f));  // +X, +Z diagonal
        // Perpendicular direction for spreading comets across the sky
        glm::vec3 perpendicular = glm::normalize(glm::vec3(1.0f, 0.0f, -1.0f));  // perpendicular to towardFing

        for (int i = 0; i < NUM_COMETS; ++i) {
            // Base position: far in the +X, +Z direction (toward FING)
            float distanceTowardFing = COMET_SKY_DISTANCE + ((float)rand() / RAND_MAX - 0.5f) * 200.0f;
            // Spread left-right perpendicular to the FING direction
            float spread = ((float)rand() / RAND_MAX - 0.5f) * COMET_SKY_SPREAD * 2.0f;
            // Y: high in the sky with some variation
            float y = COMET_SKY_HEIGHT + ((float)rand() / RAND_MAX) * 200.0f;

            glm::vec3 pos = towardFing * distanceTowardFing + perpendicular * spread;
            pos.y = y;

            // Random time offset so they don't all fall in sync
            float timeOffset = ((float)rand() / RAND_MAX) * COMET_CYCLE_TIME;

            cometInstances[i] = glm::vec4(pos.x, pos.y, pos.z, timeOffset);
        }
    }

    // Create VBO for comet instances
    GLuint cometInstanceVBO;
    glGenBuffers(1, &cometInstanceVBO);
    glBindBuffer(GL_ARRAY_BUFFER, cometInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, cometInstances.size() * sizeof(glm::vec4), cometInstances.data(), GL_STATIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // Setup instance attribute on comet mesh VAO(s)
    for (auto& mesh : cometMeshGroup.meshes) {
        glBindVertexArray(mesh.vao);
        glBindBuffer(GL_ARRAY_BUFFER, cometInstanceVBO);
        // location 3: instance data (vec4: xyz position, w timeOffset)
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(glm::vec4), (void*)0);
        glVertexAttribDivisor(3, 1);  // One per instance
        glBindVertexArray(0);
    }
    std::cout << "Setup " << NUM_COMETS << " comet instances" << std::endl;

    // Motion blur state
    glm::mat4 prevViewProjection = glm::mat4(1.0f);

    // Light direction (same as in shaders)
    glm::vec3 lightDir = glm::normalize(GameConfig::LIGHT_DIR);

    // Timing
    uint64_t prevTime = SDL_GetPerformanceCounter();
    uint64_t frequency = SDL_GetPerformanceFrequency();
    float aspectRatio = (float)GameConfig::WINDOW_WIDTH / (float)GameConfig::WINDOW_HEIGHT;

    // Register scenes
    sceneManager.registerScene(SceneType::MainMenu, std::make_unique<MainMenuScene>());
    sceneManager.registerScene(SceneType::IntroText, std::make_unique<IntroTextScene>());
    sceneManager.registerScene(SceneType::IntroCinematic, std::make_unique<IntroCinematicScene>());
    sceneManager.registerScene(SceneType::PlayGame, std::make_unique<PlayGameScene>());
    sceneManager.registerScene(SceneType::GodMode, std::make_unique<GodModeScene>());
    sceneManager.registerScene(SceneType::PauseMenu, std::make_unique<PauseMenuScene>());

    // Create scene context with all shared resources
    SceneContext sceneCtx;
    sceneCtx.registry = &registry;
    sceneCtx.gameState = &gameState;
    sceneCtx.sceneManager = &sceneManager;
    sceneCtx.inputSystem = &inputSystem;
    sceneCtx.aspectRatio = aspectRatio;

    // Systems
    sceneCtx.renderSystem = &renderSystem;
    sceneCtx.uiSystem = &uiSystem;
    sceneCtx.minimapSystem = &minimapSystem;
    sceneCtx.cinematicSystem = &cinematicSystem;
    sceneCtx.animationSystem = &animationSystem;
    sceneCtx.skeletonSystem = &skeletonSystem;
    sceneCtx.physicsSystem = &physicsSystem;
    sceneCtx.collisionSystem = &collisionSystem;
    sceneCtx.playerMovementSystem = &playerMovementSystem;
    sceneCtx.cameraOrbitSystem = &cameraOrbitSystem;
    sceneCtx.followCameraSystem = &followCameraSystem;
    sceneCtx.freeCameraSystem = &freeCameraSystem;

    // Building culling
    sceneCtx.buildingCuller = &buildingCuller;
    sceneCtx.buildingBoxMesh = &buildingBoxMesh;
    sceneCtx.buildingInstancedShader = buildingInstancedShader;
    sceneCtx.depthInstancedShader = depthInstancedShader;
    sceneCtx.buildingMaxRenderDistance = BUILDING_MAX_RENDER_DISTANCE;

    // Shaders (already pointers from AssetManager)
    sceneCtx.groundShader = groundShader;
    sceneCtx.colorShader = colorShader;
    sceneCtx.overlayShader = overlayShader;
    sceneCtx.solidOverlayShader = solidOverlayShader;
    sceneCtx.sunShader = sunShader;
    sceneCtx.cometShader = cometShader;
    sceneCtx.depthShader = depthShader;
    sceneCtx.skinnedDepthShader = skinnedDepthShader;
    sceneCtx.motionBlurShader = motionBlurShader;
    sceneCtx.toonPostShader = toonPostShader;
    sceneCtx.blitShader = blitShader;

    // Textures
    sceneCtx.snowTexture = snowTexture;
    sceneCtx.brickTexture = brickTexture;
    sceneCtx.brickNormalMap = brickNormalMap;

    // VAOs
    sceneCtx.planeVAO = planeVAO;
    sceneCtx.overlayVAO = overlayVAO;
    sceneCtx.sunVAO = sunVAO;

    // FBOs
    sceneCtx.shadowFBO = shadowFBO;
    sceneCtx.shadowDepthTexture = shadowDepthTexture;
    sceneCtx.msaaFBO = msaaFBO;
    sceneCtx.resolveFBO = resolveFBO;
    sceneCtx.resolveColorTex = resolveColorTex;
    sceneCtx.cinematicMsaaFBO = cinematicMsaaFBO;
    sceneCtx.motionBlurFBO = motionBlurFBO;
    sceneCtx.motionBlurColorTex = motionBlurColorTex;
    sceneCtx.motionBlurDepthTex = motionBlurDepthTex;
    sceneCtx.toonFBO = toonFBO;
    sceneCtx.toonColorTex = toonColorTex;

    // Motion blur state
    sceneCtx.prevViewProjection = &prevViewProjection;

    // Debug
    sceneCtx.axes = &axes;

    // Key entities
    sceneCtx.protagonist = protagonist;
    sceneCtx.camera = camera;
    sceneCtx.fingBuilding = fingBuilding;

    // FING building LOD
    sceneCtx.fingHighDetail = &fingHighDetail;
    sceneCtx.fingLowDetail = &fingLowDetail;
    sceneCtx.fingWorldCenter = fingWorldCenter;
    sceneCtx.fingWorldHalfExtents = fingWorldHalfExtents;
    sceneCtx.lodSwitchDistance = lodSwitchDistance;

    // Comet data
    sceneCtx.cometMeshGroup = &cometMeshGroup;
    sceneCtx.numComets = NUM_COMETS;
    sceneCtx.cometFallSpeed = COMET_FALL_SPEED;
    sceneCtx.cometCycleTime = COMET_CYCLE_TIME;
    sceneCtx.cometFallDistance = COMET_FALL_DISTANCE;
    sceneCtx.cometScale = COMET_SCALE;
    sceneCtx.cometFallDir = COMET_FALL_DIR;
    sceneCtx.cometColor = COMET_COLOR;

    // Light
    sceneCtx.lightDir = lightDir;

    // Building footprints for minimap
    sceneCtx.buildingFootprints = &buildingFootprints;

    // UI entities
    sceneCtx.menuOption1 = menuOption1;
    sceneCtx.menuOption2 = menuOption2;
    sceneCtx.sprintHint = sprintHint;
    sceneCtx.godModeHint = godModeHint;
    sceneCtx.pauseFogToggle = pauseFogToggle;
    sceneCtx.pauseSnowToggle = pauseSnowToggle;
    sceneCtx.pauseSnowSpeed = pauseSnowSpeed;
    sceneCtx.pauseSnowAngle = pauseSnowAngle;
    sceneCtx.pauseSnowBlur = pauseSnowBlur;
    sceneCtx.pauseToonToggle = pauseToonToggle;
    sceneCtx.pauseMenuOption = pauseMenuOption;

    // Intro text
    sceneCtx.introTextEntities = &introTextEntities;
    sceneCtx.introTexts = &introTexts;

    // Initialize render pipeline (must be after all context fields are set)
    RenderPipeline renderPipeline;
    renderPipeline.init(&sceneCtx);
    sceneCtx.renderPipeline = &renderPipeline;

    // Initialize the first scene
    sceneManager.initialize(sceneCtx);

    // Game loop
    bool running = true;
    while (running) {
        uint64_t currentTime = SDL_GetPerformanceCounter();
        float dt = (float)(currentTime - prevTime) / frequency;
        prevTime = currentTime;
        gameState.gameTime += dt;

        // Poll input events
        InputState input = inputSystem.pollEvents();
        running = !input.quit;

        // Update context with per-frame data
        sceneCtx.input = input;
        sceneCtx.dt = dt;

        // Process scene transitions (calls onExit/onEnter)
        sceneManager.processTransitions(sceneCtx);

        // Update and render current scene
        sceneManager.update(sceneCtx);
        sceneManager.render(sceneCtx);

        windowManager.swapBuffers();
    }

    // Cleanup (WindowManager handles SDL/GL cleanup automatically)
    uiSystem.cleanup();
    axes.cleanup();

    return 0;
}
