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
#include "src/scenes/RenderHelpers.h"
#include "src/procedural/BuildingGenerator.h"
#include "src/core/GameConfig.h"
#include "src/core/GameState.h"
#include "src/core/WindowManager.h"
#include "src/culling/BuildingCuller.h"

int main(int argc, char* argv[]) {
    // Initialize window and OpenGL context
    WindowManager windowManager;
    if (!windowManager.init()) {
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

    // Load assets
    LoadedModel protagonistData = loadGLB("assets/protagonist.glb");

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

    // Load modelo_fing (building/landmark) - high detail and LOD versions
    LoadedModel fingBuildingData = loadGLB("assets/modelo_fing.glb");
    LoadedModel fingBuildingLodData = loadGLB("assets/fing_lod.glb");

    // Store both mesh groups for LOD switching
    MeshGroup fingHighDetail = std::move(fingBuildingData.meshGroup);
    MeshGroup fingLowDetail = std::move(fingBuildingLodData.meshGroup);

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

    // Load comet model for sky effect
    LoadedModel cometData = loadGLB("assets/comet.glb");
    MeshGroup cometMeshGroup = std::move(cometData.meshGroup);

    // Game state - all runtime variables in one place
    GameState gameState;

    // Load brick texture for buildings (before building mesh creation)
    GLuint brickTexture = 0;
    {
        int width, height, channels;
        unsigned char* data = stbi_load("assets/textures/brick/brick_wall_006_diff_1k.jpg", &width, &height, &channels, 0);
        if (data) {
            glGenTextures(1, &brickTexture);
            glBindTexture(GL_TEXTURE_2D, brickTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(data);
            std::cout << "Loaded brick texture: assets/textures/brick/brick_wall_006_diff_1k.jpg (" << width << "x" << height << ")" << std::endl;
        } else {
            std::cerr << "Failed to load brick texture: assets/textures/brick/brick_wall_006_diff_1k.jpg" << std::endl;
        }
    }

    // Load brick normal map
    GLuint brickNormalMap = 0;
    {
        int width, height, channels;
        unsigned char* data = stbi_load("assets/textures/brick/brick_wall_006_nor_gl_1k.jpg", &width, &height, &channels, 0);
        if (data) {
            glGenTextures(1, &brickNormalMap);
            glBindTexture(GL_TEXTURE_2D, brickNormalMap);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(data);
            std::cout << "Loaded brick normal map: assets/textures/brick/brick_wall_006_nor_gl_1k.jpg (" << width << "x" << height << ")" << std::endl;
        } else {
            std::cerr << "Failed to load brick normal map: assets/textures/brick/brick_wall_006_nor_gl_1k.jpg" << std::endl;
        }
    }

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

    // Culling parameters for building visibility
    const int BUILDING_RENDER_RADIUS = GameConfig::BUILDING_RENDER_RADIUS;
    const int MAX_VISIBLE_BUILDINGS = (2 * BUILDING_RENDER_RADIUS + 1) * (2 * BUILDING_RENDER_RADIUS + 1);

    // Get building footprints for minimap
    auto buildingFootprints = BuildingGenerator::getBuildingFootprints(buildingDataList);

    // Create ground plane
    const float planeSize = GameConfig::GROUND_SIZE;
    const float texScale = GameConfig::GROUND_TEXTURE_SCALE;
    const float uvScale = planeSize * texScale;

    Entity ground = registry.create();
    Transform groundTransform;
    groundTransform.position = glm::vec3(0.0f, 0.0f, 0.0f);
    registry.addTransform(ground, groundTransform);

    // Create plane mesh (position, normal, uv) with tiled UVs
    float planeVertices[] = {
        // Position              // Normal       // UV (tiled based on world position)
        -planeSize, 0.0f, -planeSize,  0.0f, 1.0f, 0.0f,  -uvScale, -uvScale,
         planeSize, 0.0f, -planeSize,  0.0f, 1.0f, 0.0f,   uvScale, -uvScale,
         planeSize, 0.0f,  planeSize,  0.0f, 1.0f, 0.0f,   uvScale,  uvScale,
        -planeSize, 0.0f,  planeSize,  0.0f, 1.0f, 0.0f,  -uvScale,  uvScale,
    };
    unsigned short planeIndices[] = { 0, 3, 2, 0, 2, 1 };  // CCW winding when viewed from above

    GLuint planeVAO, planeVBO, planeEBO;
    glGenVertexArrays(1, &planeVAO);
    glGenBuffers(1, &planeVBO);
    glGenBuffers(1, &planeEBO);

    glBindVertexArray(planeVAO);
    glBindBuffer(GL_ARRAY_BUFFER, planeVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, planeEBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW);

    // Position (location 0)
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // Normal (location 1)
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // UV (location 2)
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    glBindVertexArray(0);

    // Load snow texture for ground plane
    GLuint snowTexture = 0;
    {
        int width, height, channels;
        // Don't flip - most textures don't need it
        unsigned char* data = stbi_load("assets/textures/snow.jpg", &width, &height, &channels, 0);
        if (data) {
            glGenTextures(1, &snowTexture);
            glBindTexture(GL_TEXTURE_2D, snowTexture);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
            glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
            glGenerateMipmap(GL_TEXTURE_2D);
            stbi_image_free(data);
            std::cout << "Loaded ground texture: assets/textures/snow.jpg (" << width << "x" << height << ")" << std::endl;
        } else {
            std::cerr << "Failed to load ground texture: assets/textures/snow.jpg" << std::endl;
        }
    }

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

    // Ground plane shader (uses model shader)
    Shader groundShader;
    groundShader.loadFromFiles("shaders/model.vert", "shaders/model.frag");

    // Instanced building shader
    Shader buildingInstancedShader;
    buildingInstancedShader.loadFromFiles("shaders/building_instanced.vert", "shaders/model.frag");

    // Instanced depth shader for shadow pass
    Shader depthInstancedShader;
    depthInstancedShader.loadFromFiles("shaders/depth_instanced.vert", "shaders/depth.frag");

    // Building culling system (octree + frustum + instanced rendering)
    BuildingCuller buildingCuller;
    // Max render distance in world units (based on grid distance)
    const float BUILDING_MAX_RENDER_DISTANCE = BUILDING_RENDER_RADIUS * BuildingGenerator::BLOCK_SIZE * 1.5f;
    buildingCuller.init(buildingDataList, MAX_VISIBLE_BUILDINGS);

    // Debug axes
    Shader colorShader;
    colorShader.loadFromFiles("shaders/color.vert", "shaders/color.frag");
    AxisRenderer axes;
    axes.init();

    // Sun billboard shader and quad
    Shader sunShader;
    sunShader.loadFromFiles("shaders/sun.vert", "shaders/sun.frag");
    GLuint sunVAO, sunVBO;
    {
        float sunQuad[] = {
            -1.0f, -1.0f,
             1.0f, -1.0f,
            -1.0f,  1.0f,
             1.0f,  1.0f
        };
        glGenVertexArrays(1, &sunVAO);
        glGenBuffers(1, &sunVBO);
        glBindVertexArray(sunVAO);
        glBindBuffer(GL_ARRAY_BUFFER, sunVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(sunQuad), sunQuad, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    // Comet shader and instance setup
    Shader cometShader;
    cometShader.loadFromFiles("shaders/comet.vert", "shaders/comet.frag");

    // Setup comet instances - spread across the distant sky
    const int NUM_COMETS = 12;
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

    // Shadow mapping setup
    Shader depthShader;
    depthShader.loadFromFiles("shaders/depth.vert", "shaders/depth.frag");
    const int SHADOW_WIDTH = GameConfig::SHADOW_MAP_SIZE, SHADOW_HEIGHT = GameConfig::SHADOW_MAP_SIZE;
    GLuint shadowFBO, shadowDepthTexture;
    {
        glGenFramebuffers(1, &shadowFBO);
        glGenTextures(1, &shadowDepthTexture);

        glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SHADOW_WIDTH, SHADOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, shadowDepthTexture, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Motion blur setup for cinematic (ping-pong FBOs)
    // GPU Gems 3 velocity-based motion blur setup
    Shader motionBlurShader;
    motionBlurShader.loadFromFiles("shaders/motion_blur.vert", "shaders/motion_blur.frag");
    GLuint motionBlurFBO;         // Single FBO for scene rendering
    GLuint motionBlurColorTex;    // Color attachment
    GLuint motionBlurDepthTex;    // Depth texture (not renderbuffer - we need to sample it!)
    {
        glGenFramebuffers(1, &motionBlurFBO);
        glGenTextures(1, &motionBlurColorTex);
        glGenTextures(1, &motionBlurDepthTex);

        // Color texture
        glBindTexture(GL_TEXTURE_2D, motionBlurColorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Depth texture (for velocity reconstruction in motion blur shader)
        glBindTexture(GL_TEXTURE_2D, motionBlurDepthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Attach to FBO
        glBindFramebuffer(GL_FRAMEBUFFER, motionBlurFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, motionBlurColorTex, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, motionBlurDepthTex, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Motion blur FBO is not complete!" << std::endl;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    const float cinematicMotionBlurStrength = GameConfig::CINEMATIC_MOTION_BLUR;
    const int motionBlurSamples = 16;  // Number of samples along velocity vector (more = smoother)
    glm::mat4 prevViewProjection = glm::mat4(1.0f);  // Previous frame's view-projection matrix

    // MSAA FBO for cinematic scene (resolved to motionBlurFBO before motion blur post-process)
    GLuint cinematicMsaaFBO;
    GLuint cinematicMsaaColorRBO;
    GLuint cinematicMsaaDepthRBO;
    {
        glGenFramebuffers(1, &cinematicMsaaFBO);
        glGenRenderbuffers(1, &cinematicMsaaColorRBO);
        glGenRenderbuffers(1, &cinematicMsaaDepthRBO);

        glBindRenderbuffer(GL_RENDERBUFFER, cinematicMsaaColorRBO);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_RGB16F,
                                          GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);

        glBindRenderbuffer(GL_RENDERBUFFER, cinematicMsaaDepthRBO);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT32F,
                                          GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);

        glBindFramebuffer(GL_FRAMEBUFFER, cinematicMsaaFBO);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, cinematicMsaaColorRBO);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, cinematicMsaaDepthRBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Cinematic MSAA FBO is not complete!" << std::endl;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // Toon/comic post-processing setup
    Shader toonPostShader;
    toonPostShader.loadFromFiles("shaders/toon_post.vert", "shaders/toon_post.frag");
    GLuint toonFBO;
    GLuint toonColorTex;
    GLuint toonDepthRBO;
    {
        glGenFramebuffers(1, &toonFBO);
        glGenTextures(1, &toonColorTex);
        glGenRenderbuffers(1, &toonDepthRBO);

        // Color texture for scene
        glBindTexture(GL_TEXTURE_2D, toonColorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Depth renderbuffer
        glBindRenderbuffer(GL_RENDERBUFFER, toonDepthRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);

        // Setup FBO
        glBindFramebuffer(GL_FRAMEBUFFER, toonFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, toonColorTex, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, toonDepthRBO);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }

    // MSAA anti-aliasing setup
    // We need two FBOs: one MSAA for rendering, one regular for resolve/display
    const int MSAA_SAMPLES = 4;
    GLuint msaaFBO;       // Multisampled FBO for rendering
    GLuint msaaColorRBO;  // Multisampled color renderbuffer
    GLuint msaaDepthRBO;  // Multisampled depth renderbuffer
    GLuint resolveFBO;    // Regular FBO for resolved result
    GLuint resolveColorTex;  // Regular texture we can sample from
    {
        // Create MSAA FBO with renderbuffers (can't sample from these directly)
        glGenFramebuffers(1, &msaaFBO);
        glGenRenderbuffers(1, &msaaColorRBO);
        glGenRenderbuffers(1, &msaaDepthRBO);

        // MSAA color renderbuffer
        glBindRenderbuffer(GL_RENDERBUFFER, msaaColorRBO);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, MSAA_SAMPLES, GL_RGB16F,
                                          GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);

        // MSAA depth renderbuffer
        glBindRenderbuffer(GL_RENDERBUFFER, msaaDepthRBO);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, MSAA_SAMPLES, GL_DEPTH24_STENCIL8,
                                          GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);

        // Attach to MSAA FBO
        glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, msaaColorRBO);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, msaaDepthRBO);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "MSAA FBO is not complete!" << std::endl;
        }

        // Create resolve FBO with regular texture (for sampling in post-process)
        glGenFramebuffers(1, &resolveFBO);
        glGenTextures(1, &resolveColorTex);

        glBindTexture(GL_TEXTURE_2D, resolveColorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, resolveFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, resolveColorTex, 0);

        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Resolve FBO is not complete!" << std::endl;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        std::cout << "MSAA " << MSAA_SAMPLES << "x enabled" << std::endl;
    }

    // Simple blit shader for final output
    Shader blitShader;
    blitShader.loadFromFiles("shaders/fullscreen.vert", "shaders/blit.frag");

    // Light direction (same as in shaders)
    glm::vec3 lightDir = glm::normalize(GameConfig::LIGHT_DIR);

    // Shadertoy overlay (fullscreen snow effect)
    Shader overlayShader;
    overlayShader.loadFromFiles("shaders/shadertoy_overlay.vert", "shaders/shadertoy_overlay.frag");

    // Solid color overlay (for menu darkening)
    Shader solidOverlayShader;
    solidOverlayShader.loadFromFiles("shaders/solid_overlay.vert", "shaders/solid_overlay.frag");
    GLuint overlayVAO, overlayVBO;
    {
        // Fullscreen quad in NDC
        float quadVertices[] = {
            -1.0f, -1.0f,
             1.0f, -1.0f,
            -1.0f,  1.0f,
             1.0f,  1.0f
        };
        glGenVertexArrays(1, &overlayVAO);
        glGenBuffers(1, &overlayVBO);
        glBindVertexArray(overlayVAO);
        glBindBuffer(GL_ARRAY_BUFFER, overlayVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    // Timing
    uint64_t prevTime = SDL_GetPerformanceCounter();
    uint64_t frequency = SDL_GetPerformanceFrequency();
    float aspectRatio = (float)GameConfig::WINDOW_WIDTH / (float)GameConfig::WINDOW_HEIGHT;

    // UI constants
    const glm::vec4 menuColorSelected(255.0f, 255.0f, 255.0f, 255.0f);
    const glm::vec4 menuColorUnselected(128.0f, 128.0f, 128.0f, 255.0f);
    const int PAUSE_MENU_ITEMS = 7;  // Fog, Snow, Snow Speed, Snow Angle, Snow Blur, Toon, Back

    // Game loop
    bool running = true;
    while (running) {
        uint64_t currentTime = SDL_GetPerformanceCounter();
        float dt = (float)(currentTime - prevTime) / frequency;
        prevTime = currentTime;
        gameState.gameTime += dt;  // Accumulate time for shader effects

        // Poll input events
        InputState input = inputSystem.pollEvents();
        running = !input.quit;

        // Handle scene changes
        if (sceneManager.hasSceneChanged()) {
            SceneType scene = sceneManager.current();

            // Hide all scene-specific UI
            registry.getUIText(menuOption1)->visible = false;
            registry.getUIText(menuOption2)->visible = false;
            registry.getUIText(sprintHint)->visible = false;
            registry.getUIText(godModeHint)->visible = false;
            registry.getUIText(pauseFogToggle)->visible = false;
            registry.getUIText(pauseSnowToggle)->visible = false;
            registry.getUIText(pauseSnowSpeed)->visible = false;
            registry.getUIText(pauseSnowAngle)->visible = false;
            registry.getUIText(pauseSnowBlur)->visible = false;
            registry.getUIText(pauseToonToggle)->visible = false;
            registry.getUIText(pauseMenuOption)->visible = false;
            // Hide all intro text entities
            for (auto& entity : introTextEntities) {
                auto* text = registry.getUIText(entity);
                if (text) text->visible = false;
            }

            if (scene == SceneType::MainMenu) {
                inputSystem.captureMouse(false);
                registry.getUIText(menuOption1)->visible = true;
                registry.getUIText(menuOption2)->visible = true;
                uiSystem.clearCache();  // Clear cache to update colors
            }
            else if (scene == SceneType::IntroText) {
                inputSystem.captureMouse(false);
                // FONT TEST: Just show all pre-set text (no typewriter)
                std::cout << "=== ENTERING INTRO TEXT SCENE ===" << std::endl;
                std::cout << "Total intro entities: " << introTextEntities.size() << std::endl;
                int idx = 0;
                for (auto& entity : introTextEntities) {
                    auto* text = registry.getUIText(entity);
                    if (text) {
                        text->visible = true;
                        std::cout << idx << ": '" << text->text << "' font=" << text->fontId << " size=" << text->fontSize << std::endl;
                    } else {
                        std::cout << idx << ": NULL TEXT!" << std::endl;
                    }
                    idx++;
                }
                uiSystem.clearCache();
            }
            else if (scene == SceneType::IntroCinematic) {
                inputSystem.captureMouse(false);  // No mouse control during cinematic

                // Reset protagonist position
                auto* pt = registry.getTransform(protagonist);
                if (pt) {
                    pt->position = GameConfig::INTRO_CHARACTER_POS;
                }
                // Character faces toward FING (yaw 225 = toward +X,+Z diagonal)
                auto* pf = registry.getFacingDirection(protagonist);
                if (pf) {
                    pf->yaw = 225.0f;
                }

                // Reset motion blur accumulation for fresh start
                gameState.motionBlurInitialized = false;
                gameState.motionBlurPingPong = 0;

                // Start the cinematic
                cinematicSystem.start(registry);
            }
            else if (scene == SceneType::PlayGame) {
                inputSystem.captureMouse(true);
                registry.getUIText(sprintHint)->visible = true;

                // Only reset protagonist position when NOT coming from pause menu
                // (pause menu should resume where player left off)
                if (sceneManager.previous() != SceneType::PauseMenu) {
                    auto* pt = registry.getTransform(protagonist);
                    if (pt) {
                        pt->position = GameConfig::INTRO_CHARACTER_POS;
                    }
                    auto* pf = registry.getFacingDirection(protagonist);
                    if (pf) {
                        pf->yaw = 225.0f;  // Face toward FING (+X,+Z diagonal)
                    }
                }
            }
            else if (scene == SceneType::GodMode) {
                inputSystem.captureMouse(true);
                registry.getUIText(godModeHint)->visible = true;

                // Only reset camera position when NOT coming from pause menu
                if (sceneManager.previous() != SceneType::PauseMenu) {
                    auto* ct = registry.getTransform(camera);
                    if (ct) {
                        ct->position = glm::vec3(5.0f, 3.0f, 5.0f);
                    }
                    freeCameraSystem.setPosition(glm::vec3(5.0f, 3.0f, 5.0f), -45.0f, -15.0f);
                }
            }
            else if (scene == SceneType::PauseMenu) {
                inputSystem.captureMouse(false);
                gameState.pauseMenuSelection = 0;  // Reset to first option
                registry.getUIText(pauseFogToggle)->visible = true;
                registry.getUIText(pauseSnowToggle)->visible = true;
                registry.getUIText(pauseSnowSpeed)->visible = true;
                registry.getUIText(pauseSnowAngle)->visible = true;
                registry.getUIText(pauseSnowBlur)->visible = true;
                registry.getUIText(pauseToonToggle)->visible = true;
                registry.getUIText(pauseMenuOption)->visible = true;
                // Update colors based on selection
                registry.getUIText(pauseFogToggle)->color = menuColorSelected;
                registry.getUIText(pauseSnowToggle)->color = menuColorUnselected;
                registry.getUIText(pauseSnowSpeed)->color = menuColorUnselected;
                registry.getUIText(pauseSnowAngle)->color = menuColorUnselected;
                registry.getUIText(pauseSnowBlur)->color = menuColorUnselected;
                registry.getUIText(pauseToonToggle)->color = menuColorUnselected;
                registry.getUIText(pauseMenuOption)->color = menuColorUnselected;
                uiSystem.clearCache();
            }
        }

        SceneType currentScene = sceneManager.current();

        // Scene-specific update logic
        if (currentScene == SceneType::MainMenu) {
            // Menu navigation
            if (input.upPressed || input.downPressed) {
                gameState.menuSelection = 1 - gameState.menuSelection;  // Toggle between 0 and 1

                // Update menu colors
                auto* text1 = registry.getUIText(menuOption1);
                auto* text2 = registry.getUIText(menuOption2);
                if (text1 && text2) {
                    text1->color = (gameState.menuSelection == 0) ? menuColorSelected : menuColorUnselected;
                    text2->color = (gameState.menuSelection == 1) ? menuColorSelected : menuColorUnselected;
                    uiSystem.clearCache();  // Force re-render with new colors
                }
            }

            if (input.enterPressed) {
                if (gameState.menuSelection == 0) {
                    sceneManager.switchTo(SceneType::IntroText);
                } else {
                    sceneManager.switchTo(SceneType::GodMode);
                }
            }

            // Update game time for comets animation
            gameState.gameTime += dt;

            // Static camera backdrop
            const glm::vec3 menuCamPos = glm::vec3(-4.82f, 4.57f, 19.15f);
            const glm::vec3 menuCamLookAt = glm::vec3(-4.16f, 4.81f, 19.85f);
            glm::mat4 menuView = glm::lookAt(menuCamPos, menuCamLookAt, glm::vec3(0.0f, 1.0f, 0.0f));

            auto* cam = registry.getCamera(camera);
            glm::mat4 projection = cam ? cam->projectionMatrix(aspectRatio) : glm::mat4(1.0f);

            // Clear with sky color
            glClearColor(0.2f, 0.2f, 0.22f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Render FING model (via render system - no buildings, just the FING entity)
            renderSystem.setFogEnabled(gameState.fogEnabled);
            renderSystem.updateWithView(registry, aspectRatio, menuView);

            // Render ground plane (no buildings, no shadows)
            RenderHelpers::renderGroundPlane(groundShader, menuView, projection, glm::mat4(1.0f),
                lightDir, menuCamPos, gameState.fogEnabled, false, snowTexture, 0, planeVAO);

            // Render snow overlay
            RenderHelpers::renderSnowOverlay(overlayShader, overlayVAO, gameState);

            // Render falling comets
            {
                glEnable(GL_DEPTH_TEST);
                glDepthMask(GL_FALSE);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                cometShader.use();
                cometShader.setMat4("uView", menuView);
                cometShader.setMat4("uProjection", projection);
                cometShader.setFloat("uTime", gameState.gameTime);
                cometShader.setVec3("uCameraPos", menuCamPos);
                cometShader.setFloat("uFallSpeed", COMET_FALL_SPEED);
                cometShader.setFloat("uCycleTime", COMET_CYCLE_TIME);
                cometShader.setFloat("uFallDistance", COMET_FALL_DISTANCE);
                // More diagonal fall for menu backdrop (nearly horizontal)
                cometShader.setVec3("uFallDirection", glm::normalize(glm::vec3(0.85f, -0.12f, 0.4f)));
                cometShader.setFloat("uScale", COMET_SCALE);
                cometShader.setInt("uDebugMode", 0);
                cometShader.setFloat("uTrailStretch", 15.0f);
                cometShader.setFloat("uGroundY", 0.0f);  // Ground plane at Y=0
                cometShader.setInt("uHasTexture", 0);
                cometShader.setVec3("uCometColor", glm::vec3(1.0f, 0.4f, 0.1f));

                for (const auto& mesh : cometMeshGroup.meshes) {
                    glBindVertexArray(mesh.vao);
                    glDrawElementsInstanced(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr, NUM_COMETS);
                }

                glDepthMask(GL_TRUE);
            }

            // Draw 75% black overlay
            glDisable(GL_DEPTH_TEST);
            glEnable(GL_BLEND);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

            solidOverlayShader.use();
            solidOverlayShader.setVec4("uColor", glm::vec4(0.0f, 0.0f, 0.0f, 0.85f));
            glBindVertexArray(overlayVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            glEnable(GL_DEPTH_TEST);

            // Render UI on top
            uiSystem.update(registry, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
        }
        else if (currentScene == SceneType::IntroText) {
            // Skip intro with Enter or Escape
            if (input.enterPressed || input.escapePressed) {
                sceneManager.switchTo(SceneType::IntroCinematic);
            }

            // Typewriter effect update
            if (!gameState.introAllComplete) {
                if (gameState.introLineComplete) {
                    // Waiting between lines
                    gameState.introLinePauseTimer += dt;
                    if (gameState.introLinePauseTimer >= introLineDelay) {
                        gameState.introLinePauseTimer = 0.0f;
                        gameState.introLineComplete = false;
                        gameState.introCurrentLine++;
                        gameState.introCurrentChar = 0;
                        if (gameState.introCurrentLine >= (int)introTexts.size()) {
                            gameState.introAllComplete = true;
                        }
                    }
                } else if (gameState.introCurrentLine < (int)introTexts.size()) {
                    // Typing characters
                    gameState.introTypewriterTimer += dt;
                    while (gameState.introTypewriterTimer >= introCharDelay && gameState.introCurrentChar < introTexts[gameState.introCurrentLine].length()) {
                        gameState.introTypewriterTimer -= introCharDelay;
                        gameState.introCurrentChar++;
                        // Update the text entity
                        auto* text = registry.getUIText(introTextEntities[gameState.introCurrentLine]);
                        if (text) {
                            text->text = introTexts[gameState.introCurrentLine].substr(0, gameState.introCurrentChar);
                            uiSystem.clearCache();
                        }
                    }
                    // Check if line is complete
                    if (gameState.introCurrentChar >= introTexts[gameState.introCurrentLine].length()) {
                        gameState.introLineComplete = true;
                        gameState.introLinePauseTimer = 0.0f;
                    }
                }
            } else {
                // All text complete, wait a moment then transition
                gameState.introLinePauseTimer += dt;
                if (gameState.introLinePauseTimer >= 2.0f) {  // 2 second pause after all text
                    sceneManager.switchTo(SceneType::IntroCinematic);
                }
            }

            // Clear with black for intro text
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Only render UI
            uiSystem.update(registry, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
        }
        else if (currentScene == SceneType::IntroCinematic) {
            // Skip cinematic with Enter or Escape
            if (input.enterPressed || input.escapePressed) {
                cinematicSystem.stop(registry);
                sceneManager.switchTo(SceneType::PlayGame);
            }

            // Update cinematic - when complete, switch to gameplay
            if (!cinematicSystem.update(registry, dt)) {
                if (cinematicSystem.isComplete()) {
                    sceneManager.switchTo(SceneType::PlayGame);
                }
            }

            // Still update animations for visual effect
            animationSystem.update(registry, dt);
            skeletonSystem.update(registry);

            // Get cinematic view matrix
            auto* protagonistT = registry.getTransform(protagonist);
            auto* cam = registry.getCamera(camera);
            auto* camT = registry.getTransform(camera);
            glm::mat4 cinematicView = cinematicSystem.getViewMatrix(registry);
            glm::mat4 projection = cam ? cam->projectionMatrix(aspectRatio) : glm::mat4(1.0f);
            glm::vec3 cameraPos = cinematicSystem.getCurrentCameraPosition();

            // Update building culling with octree + frustum
            buildingCuller.update(cinematicView, projection, cameraPos, BUILDING_MAX_RENDER_DISTANCE);

            // === SHADOW PASS (cinematic) ===
            glm::vec3 focusPoint = protagonistT ? protagonistT->position : glm::vec3(0.0f);
            glm::mat4 lightSpaceMatrix = RenderHelpers::computeLightSpaceMatrix(focusPoint, lightDir);

            glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
            glClear(GL_DEPTH_BUFFER_BIT);

            // Update shadow casters based on light frustum (not camera frustum!)
            // This ensures buildings behind camera still cast shadows
            buildingCuller.updateShadowCasters(lightSpaceMatrix, cameraPos, BUILDING_MAX_RENDER_DISTANCE);

            // Render buildings to shadow map using instanced rendering
            buildingCuller.renderShadows(buildingBoxMesh, depthInstancedShader, lightSpaceMatrix);

            // Render fing building to shadow map
            {
                auto* t = registry.getTransform(fingBuilding);
                auto* mg = registry.getMeshGroup(fingBuilding);
                if (t && mg) {
                    depthShader.use();
                    depthShader.setMat4("uLightSpaceMatrix", lightSpaceMatrix);
                    depthShader.setMat4("uModel", t->matrix());
                    for (const auto& mesh : mg->meshes) {
                        glBindVertexArray(mesh.vao);
                        glDrawElements(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr);
                    }
                }
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);

            // === RENDER CINEMATIC SCENE TO MSAA FBO ===
            // Compute current view-projection matrix for motion blur
            glm::mat4 currentViewProjection = projection * cinematicView;

            // Render to MSAA FBO for anti-aliased geometry
            glBindFramebuffer(GL_FRAMEBUFFER, cinematicMsaaFBO);
            glViewport(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);

            glClearColor(0.2f, 0.2f, 0.22f, 1.0f);  // Dark gray sky
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Draw debug axes
            if (cam && camT) {
                glm::mat4 vp = projection * cinematicView;
                colorShader.use();
                colorShader.setMat4("uMVP", vp);
                axes.draw();
            }

            // Render scene with cinematic view and shadows
            RenderHelpers::setupRenderSystem(renderSystem, gameState.fogEnabled, true, shadowDepthTexture, lightSpaceMatrix);
            renderSystem.updateWithView(registry, aspectRatio, cinematicView);

            // Render buildings using instanced rendering
            {
                BuildingRenderParams buildingParams;
                buildingParams.view = cinematicView;
                buildingParams.projection = projection;
                buildingParams.lightSpaceMatrix = lightSpaceMatrix;
                buildingParams.lightDir = lightDir;
                buildingParams.viewPos = cameraPos;
                buildingParams.texture = brickTexture;
                buildingParams.normalMap = brickNormalMap;
                buildingParams.shadowMap = shadowDepthTexture;
                buildingParams.textureScale = GameConfig::BUILDING_TEXTURE_SCALE;
                buildingParams.fogEnabled = gameState.fogEnabled;
                buildingParams.shadowsEnabled = true;
                buildingCuller.render(buildingBoxMesh, buildingInstancedShader, buildingParams);
            }

            // Render ground plane with shadows
            RenderHelpers::renderGroundPlane(groundShader, cinematicView, projection, lightSpaceMatrix,
                lightDir, cinematicSystem.getCurrentCameraPosition(),
                gameState.fogEnabled, true, snowTexture, shadowDepthTexture, planeVAO);

            // Render snow overlay to the FBO
            RenderHelpers::renderSnowOverlay(overlayShader, overlayVAO, gameState);

            // Render falling comets in sky (Cinematic)
            {
                glEnable(GL_DEPTH_TEST);   // Read depth (occluded by closer geometry)
                glDepthMask(GL_FALSE);     // Don't write depth
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                cometShader.use();
                cometShader.setMat4("uView", cinematicView);
                cometShader.setMat4("uProjection", projection);
                cometShader.setFloat("uTime", gameState.gameTime);
                cometShader.setVec3("uCameraPos", cameraPos);
                cometShader.setFloat("uFallSpeed", COMET_FALL_SPEED);
                cometShader.setFloat("uCycleTime", COMET_CYCLE_TIME);
                cometShader.setFloat("uFallDistance", COMET_FALL_DISTANCE);
                cometShader.setVec3("uFallDirection", COMET_FALL_DIR);
                cometShader.setFloat("uScale", COMET_SCALE);
                cometShader.setVec3("uCometColor", COMET_COLOR);
                cometShader.setInt("uDebugMode", 0);  // Set to 1 to debug comet orientation
                cometShader.setInt("uTexture", 0);   // Texture unit 0
                cometShader.setFloat("uTrailStretch", 15.0f);  // Motion blur trail stretch (exaggerated)
                cometShader.setFloat("uGroundY", 0.0f);  // Ground plane at Y=0

                for (const auto& mesh : cometMeshGroup.meshes) {
                    // Bind texture if available
                    if (mesh.texture != 0) {
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, mesh.texture);
                        cometShader.setInt("uHasTexture", 1);
                    } else {
                        cometShader.setInt("uHasTexture", 0);
                    }

                    glBindVertexArray(mesh.vao);
                    glDrawElementsInstanced(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr, NUM_COMETS);
                }
                glBindVertexArray(0);

                glDisable(GL_BLEND);
                glDepthMask(GL_TRUE);
                glEnable(GL_DEPTH_TEST);
            }

            // === RESOLVE MSAA TO MOTION BLUR FBO ===
            // The motion blur shader needs to sample the texture, so resolve MSAA first
            glBindFramebuffer(GL_READ_FRAMEBUFFER, cinematicMsaaFBO);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, motionBlurFBO);
            glBlitFramebuffer(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT,
                              0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT,
                              GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

            // === GPU GEMS VELOCITY-BASED MOTION BLUR POST-PROCESS ===
            // Render motion-blurred result to MSAA FBO (for final resolve)
            glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO);
            glViewport(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);

            motionBlurShader.use();

            // Bind color buffer (scene we just rendered)
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, motionBlurColorTex);
            motionBlurShader.setInt("uColorBuffer", 0);

            // Bind depth buffer (for world position reconstruction)
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, motionBlurDepthTex);
            motionBlurShader.setInt("uDepthBuffer", 1);

            // Pass matrices for velocity computation
            motionBlurShader.setMat4("uViewProjection", currentViewProjection);
            motionBlurShader.setMat4("uInvViewProjection", glm::inverse(currentViewProjection));

            // On first frame, use current matrix as previous (no blur)
            if (!gameState.motionBlurInitialized) {
                prevViewProjection = currentViewProjection;
            }
            motionBlurShader.setMat4("uPrevViewProjection", prevViewProjection);

            // Blur parameters
            motionBlurShader.setFloat("uBlurStrength", cinematicMotionBlurStrength);
            motionBlurShader.setInt("uNumSamples", motionBlurSamples);

            glBindVertexArray(overlayVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindVertexArray(0);

            glEnable(GL_DEPTH_TEST);

            // Store current view-projection for next frame
            prevViewProjection = currentViewProjection;
            gameState.motionBlurInitialized = true;
        }
        else if (currentScene == SceneType::PlayGame) {
            // Open pause menu on escape
            if (input.escapePressed) {
                sceneManager.switchTo(SceneType::PauseMenu);
            }

            // Update gameplay systems
            cameraOrbitSystem.update(registry, input.mouseX, input.mouseY);

            // Create AABB for FING building (computed from model vertices)
            // Used for both player and camera collision
            auto* fingT = registry.getTransform(fingBuilding);
            AABB fingAABB;
            if (fingT) {
                // Use pre-computed world-space bounds from model loading
                fingAABB = AABB::fromCenterExtents(fingWorldCenter, fingWorldHalfExtents);
            }

            // Player movement with building collision (no FING collision - player can walk through it)
            playerMovementSystem.update(registry, dt, &buildingCuller, nullptr);

            // Camera with collision detection (no FING collision)
            followCameraSystem.updateWithCollision(registry, buildingCuller, nullptr);

            physicsSystem.update(registry, dt);
            collisionSystem.update(registry);
            animationSystem.update(registry, dt);
            skeletonSystem.update(registry);

            // LOD updates
            auto* protagonistT = registry.getTransform(protagonist);
            auto* fingBuildingT = registry.getTransform(fingBuilding);
            if (protagonistT) {
                RenderHelpers::updateFingLOD(registry, gameState, fingBuilding, protagonistT->position,
                    fingHighDetail, fingLowDetail, lodSwitchDistance);
            }

            // Compute camera matrices before shadow pass
            auto* cam = registry.getCamera(camera);
            auto* camT = registry.getTransform(camera);
            auto* protagonistFacing = registry.getFacingDirection(protagonist);
            auto* ft = registry.getFollowTarget(camera);
            glm::mat4 playView;
            glm::mat4 projection;
            if (cam && camT && protagonistT && protagonistFacing && ft) {
                glm::vec3 lookAtPos = FollowCameraSystem::getLookAtPosition(*protagonistT, *ft, protagonistFacing->yaw);
                playView = glm::lookAt(camT->position, lookAtPos, glm::vec3(0.0f, 1.0f, 0.0f));
                projection = cam->projectionMatrix(aspectRatio);
            }

            // Update building culling with octree + frustum
            glm::vec3 cameraPos = camT ? camT->position : glm::vec3(0.0f);
            buildingCuller.update(playView, projection, cameraPos, BUILDING_MAX_RENDER_DISTANCE);

            // === SHADOW PASS ===
            // Compute light space matrix centered on player
            float orthoSize = GameConfig::SHADOW_ORTHO_SIZE;
            glm::vec3 lightPos = (protagonistT ? protagonistT->position : glm::vec3(0.0f)) + lightDir * GameConfig::SHADOW_DISTANCE;
            glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, GameConfig::SHADOW_NEAR, GameConfig::SHADOW_FAR);
            glm::mat4 lightView = glm::lookAt(lightPos, protagonistT ? protagonistT->position : glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 lightSpaceMatrix = lightProjection * lightView;

            glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
            glClear(GL_DEPTH_BUFFER_BIT);

            // Update shadow casters based on light frustum (not camera frustum!)
            // This ensures buildings behind camera still cast shadows
            buildingCuller.updateShadowCasters(lightSpaceMatrix, cameraPos, BUILDING_MAX_RENDER_DISTANCE);

            // Render buildings to shadow map using instanced rendering
            buildingCuller.renderShadows(buildingBoxMesh, depthInstancedShader, lightSpaceMatrix);

            // Render fing building to shadow map
            {
                auto* t = registry.getTransform(fingBuilding);
                auto* mg = registry.getMeshGroup(fingBuilding);
                if (t && mg) {
                    depthShader.use();
                    depthShader.setMat4("uLightSpaceMatrix", lightSpaceMatrix);
                    depthShader.setMat4("uModel", t->matrix());
                    for (const auto& mesh : mg->meshes) {
                        glBindVertexArray(mesh.vao);
                        glDrawElements(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr);
                    }
                }
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);

            // === MAIN RENDER PASS ===
            // Render to appropriate FBO: toonFBO for toon shading, msaaFBO otherwise
            if (gameState.toonShadingEnabled) {
                glBindFramebuffer(GL_FRAMEBUFFER, toonFBO);
            } else {
                glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO);
            }
            glClearColor(0.2f, 0.2f, 0.22f, 1.0f);  // Dark gray sky
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Draw debug axes
            if (cam && camT && protagonistT && protagonistFacing && ft) {
                glm::mat4 vp = projection * playView;
                colorShader.use();
                colorShader.setMat4("uMVP", vp);
                axes.draw();
            }

            // Render scene with shadows
            RenderHelpers::setupRenderSystem(renderSystem, gameState.fogEnabled, true, shadowDepthTexture, lightSpaceMatrix);
            renderSystem.update(registry, aspectRatio);

            // Render buildings using instanced rendering
            {
                BuildingRenderParams buildingParams;
                buildingParams.view = playView;
                buildingParams.projection = projection;
                buildingParams.lightSpaceMatrix = lightSpaceMatrix;
                buildingParams.lightDir = lightDir;
                buildingParams.viewPos = cameraPos;
                buildingParams.texture = brickTexture;
                buildingParams.normalMap = brickNormalMap;
                buildingParams.shadowMap = shadowDepthTexture;
                buildingParams.textureScale = GameConfig::BUILDING_TEXTURE_SCALE;
                buildingParams.fogEnabled = gameState.fogEnabled;
                buildingParams.shadowsEnabled = true;
                buildingCuller.render(buildingBoxMesh, buildingInstancedShader, buildingParams);
            }

            // Render ground plane with shadows
            RenderHelpers::renderGroundPlane(groundShader, playView, projection, lightSpaceMatrix,
                lightDir, camT->position, gameState.fogEnabled, true,
                snowTexture, shadowDepthTexture, planeVAO);

            // Render sun billboard
            if (cam && camT) {
                glm::vec3 sunWorldPos = camT->position + lightDir * 400.0f;
                glDisable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                sunShader.use();
                sunShader.setMat4("uView", playView);
                sunShader.setMat4("uProjection", projection);
                sunShader.setVec3("uSunWorldPos", sunWorldPos);
                sunShader.setFloat("uSize", 30.0f);

                glBindVertexArray(sunVAO);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBindVertexArray(0);

                glDisable(GL_BLEND);
                glEnable(GL_DEPTH_TEST);
            }

            // Render falling comets in sky
            if (cam && camT) {
                glEnable(GL_DEPTH_TEST);   // Read depth (occluded by closer geometry)
                glDepthMask(GL_FALSE);     // Don't write depth
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                cometShader.use();
                cometShader.setMat4("uView", playView);
                cometShader.setMat4("uProjection", projection);
                cometShader.setFloat("uTime", gameState.gameTime);
                cometShader.setVec3("uCameraPos", camT->position);
                cometShader.setFloat("uFallSpeed", COMET_FALL_SPEED);
                cometShader.setFloat("uCycleTime", COMET_CYCLE_TIME);
                cometShader.setFloat("uFallDistance", COMET_FALL_DISTANCE);
                cometShader.setVec3("uFallDirection", COMET_FALL_DIR);
                cometShader.setFloat("uScale", COMET_SCALE);
                cometShader.setVec3("uCometColor", COMET_COLOR);
                cometShader.setInt("uDebugMode", 0);  // Set to 1 to debug comet orientation
                cometShader.setInt("uTexture", 0);   // Texture unit 0
                cometShader.setFloat("uTrailStretch", 15.0f);  // Motion blur trail stretch (exaggerated)
                cometShader.setFloat("uGroundY", 0.0f);  // Ground plane at Y=0

                for (const auto& mesh : cometMeshGroup.meshes) {
                    // Bind texture if available
                    if (mesh.texture != 0) {
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, mesh.texture);
                        cometShader.setInt("uHasTexture", 1);
                    } else {
                        cometShader.setInt("uHasTexture", 0);
                    }

                    glBindVertexArray(mesh.vao);
                    glDrawElementsInstanced(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr, NUM_COMETS);
                }
                glBindVertexArray(0);

                glDisable(GL_BLEND);
                glDepthMask(GL_TRUE);
                glEnable(GL_DEPTH_TEST);
            }

            // Render snow overlay (fullscreen 2D effect)
            RenderHelpers::renderSnowOverlay(overlayShader, overlayVAO, gameState);

            // === TOON POST-PROCESSING ===
            if (gameState.toonShadingEnabled) {
                // Switch to MSAA FBO for toon output
                glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Apply toon shader
                toonPostShader.use();
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, toonColorTex);
                toonPostShader.setInt("uSceneTex", 0);
                toonPostShader.setVec2("uTexelSize", glm::vec2(1.0f / GameConfig::WINDOW_WIDTH, 1.0f / GameConfig::WINDOW_HEIGHT));

                glDisable(GL_DEPTH_TEST);
                glBindVertexArray(overlayVAO);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBindVertexArray(0);
                glEnable(GL_DEPTH_TEST);
            }

            // Build marker positions for minimap
            std::vector<glm::vec3> minimapMarkers;
            if (fingBuildingT) {
                minimapMarkers.push_back(fingBuildingT->position);
            }
            minimapSystem.render(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT, protagonistFacing ? protagonistFacing->yaw : 0.0f,
                                 uiSystem.fonts(), uiSystem.textCache(),
                                 protagonistT ? protagonistT->position : glm::vec3(0.0f), minimapMarkers,
                                 buildingFootprints);
            uiSystem.update(registry, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
        }
        else if (currentScene == SceneType::GodMode) {
            // Open pause menu on escape
            if (input.escapePressed) {
                sceneManager.switchTo(SceneType::PauseMenu);
            }

            // Free camera control
            freeCameraSystem.update(registry, dt, input.mouseX, input.mouseY);

            // Debug: Write camera position and lookAt to file when P is pressed
            if (input.pPressed) {
                auto* camT_debug = registry.getTransform(camera);
                if (camT_debug) {
                    glm::vec3 pos = camT_debug->position;
                    glm::vec3 lookAt = pos + freeCameraSystem.forward();
                    FILE* f = fopen("camera_debug.txt", "a");
                    if (f) {
                        fprintf(f, "Camera Position: (%.2f, %.2f, %.2f)\n", pos.x, pos.y, pos.z);
                        fprintf(f, "Camera LookAt:   (%.2f, %.2f, %.2f)\n", lookAt.x, lookAt.y, lookAt.z);
                        fprintf(f, "Yaw: %.2f, Pitch: %.2f\n\n", freeCameraSystem.yaw(), freeCameraSystem.pitch());
                        fclose(f);
                    }
                }
            }

            // Still update animations for visual effect
            animationSystem.update(registry, dt);
            skeletonSystem.update(registry);

            // LOD update for fing building (based on camera distance in god mode)
            auto* camT_lod = registry.getTransform(camera);
            if (camT_lod) {
                RenderHelpers::updateFingLOD(registry, gameState, fingBuilding, camT_lod->position,
                    fingHighDetail, fingLowDetail, lodSwitchDistance);
            }

            // Render
            // Render to appropriate FBO: toonFBO for toon shading, msaaFBO otherwise
            if (gameState.toonShadingEnabled) {
                glBindFramebuffer(GL_FRAMEBUFFER, toonFBO);
            } else {
                glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO);
            }
            glClearColor(0.2f, 0.2f, 0.22f, 1.0f);  // Dark gray sky
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Draw debug axes with free camera view
            auto* cam = registry.getCamera(camera);
            auto* camT = registry.getTransform(camera);
            if (cam && camT) {
                glm::mat4 view = freeCameraSystem.getViewMatrix(camT->position);
                glm::mat4 vp = cam->projectionMatrix(aspectRatio) * view;
                colorShader.use();
                colorShader.setMat4("uMVP", vp);
                axes.draw();

                renderSystem.setFogEnabled(gameState.fogEnabled);
                renderSystem.updateWithView(registry, aspectRatio, view);

                // Render ground plane
                glm::mat4 projection = cam->projectionMatrix(aspectRatio);
                glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
                groundShader.use();
                groundShader.setMat4("uView", view);
                groundShader.setMat4("uProjection", projection);
                groundShader.setMat4("uModel", glm::mat4(1.0f));
                groundShader.setVec3("uLightDir", lightDir);
                groundShader.setVec3("uViewPos", camT->position);
                groundShader.setInt("uHasTexture", 1);
                groundShader.setInt("uFogEnabled", gameState.fogEnabled ? 1 : 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, snowTexture);
                groundShader.setInt("uTexture", 0);
                glBindVertexArray(planeVAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
                glBindVertexArray(0);

                // Update and render buildings (GodMode)
                buildingCuller.update(view, projection, camT->position, BUILDING_MAX_RENDER_DISTANCE);
                {
                    BuildingRenderParams buildingParams;
                    buildingParams.view = view;
                    buildingParams.projection = projection;
                    buildingParams.lightSpaceMatrix = glm::mat4(1.0f);  // No shadows in god mode for simplicity
                    buildingParams.lightDir = lightDir;
                    buildingParams.viewPos = camT->position;
                    buildingParams.texture = brickTexture;
                    buildingParams.normalMap = brickNormalMap;
                    buildingParams.shadowMap = 0;
                    buildingParams.textureScale = GameConfig::BUILDING_TEXTURE_SCALE;
                    buildingParams.fogEnabled = gameState.fogEnabled;
                    buildingParams.shadowsEnabled = false;
                    buildingCuller.render(buildingBoxMesh, buildingInstancedShader, buildingParams);
                }

                // Render falling comets in sky (GodMode)
                // view and projection already defined above

                glEnable(GL_DEPTH_TEST);   // Read depth (occluded by closer geometry)
                glDepthMask(GL_FALSE);     // Don't write depth
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                cometShader.use();
                cometShader.setMat4("uView", view);
                cometShader.setMat4("uProjection", projection);
                cometShader.setFloat("uTime", gameState.gameTime);
                cometShader.setVec3("uCameraPos", camT->position);
                cometShader.setFloat("uFallSpeed", COMET_FALL_SPEED);
                cometShader.setFloat("uCycleTime", COMET_CYCLE_TIME);
                cometShader.setFloat("uFallDistance", COMET_FALL_DISTANCE);
                cometShader.setVec3("uFallDirection", COMET_FALL_DIR);
                cometShader.setFloat("uScale", COMET_SCALE);
                cometShader.setVec3("uCometColor", COMET_COLOR);
                cometShader.setInt("uDebugMode", 0);
                cometShader.setInt("uTexture", 0);   // Texture unit 0
                cometShader.setFloat("uTrailStretch", 15.0f);  // Motion blur trail stretch (exaggerated)

                for (const auto& mesh : cometMeshGroup.meshes) {
                    // Bind texture if available
                    if (mesh.texture != 0) {
                        glActiveTexture(GL_TEXTURE0);
                        glBindTexture(GL_TEXTURE_2D, mesh.texture);
                        cometShader.setInt("uHasTexture", 1);
                    } else {
                        cometShader.setInt("uHasTexture", 0);
                    }

                    glBindVertexArray(mesh.vao);
                    glDrawElementsInstanced(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr, NUM_COMETS);
                }
                glBindVertexArray(0);

                glDisable(GL_BLEND);
                glDepthMask(GL_TRUE);
                glEnable(GL_DEPTH_TEST);
            }

            // === TOON POST-PROCESSING ===
            if (gameState.toonShadingEnabled) {
                // Switch to MSAA FBO for toon output
                glBindFramebuffer(GL_FRAMEBUFFER, msaaFBO);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

                // Apply toon shader
                toonPostShader.use();
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, toonColorTex);
                toonPostShader.setInt("uSceneTex", 0);
                toonPostShader.setVec2("uTexelSize", glm::vec2(1.0f / GameConfig::WINDOW_WIDTH, 1.0f / GameConfig::WINDOW_HEIGHT));

                glDisable(GL_DEPTH_TEST);
                glBindVertexArray(overlayVAO);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBindVertexArray(0);
                glEnable(GL_DEPTH_TEST);
            }

            minimapSystem.render(GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
            uiSystem.update(registry, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
        }
        else if (currentScene == SceneType::PauseMenu) {
            // Resume game on escape
            if (input.escapePressed) {
                sceneManager.switchTo(sceneManager.previous());
            }

            // Menu navigation
            if (input.upPressed) {
                gameState.pauseMenuSelection = (gameState.pauseMenuSelection - 1 + PAUSE_MENU_ITEMS) % PAUSE_MENU_ITEMS;
            }
            if (input.downPressed) {
                gameState.pauseMenuSelection = (gameState.pauseMenuSelection + 1) % PAUSE_MENU_ITEMS;
            }

            // Update menu colors based on selection
            if (input.upPressed || input.downPressed) {
                registry.getUIText(pauseFogToggle)->color = (gameState.pauseMenuSelection == 0) ? menuColorSelected : menuColorUnselected;
                registry.getUIText(pauseSnowToggle)->color = (gameState.pauseMenuSelection == 1) ? menuColorSelected : menuColorUnselected;
                registry.getUIText(pauseSnowSpeed)->color = (gameState.pauseMenuSelection == 2) ? menuColorSelected : menuColorUnselected;
                registry.getUIText(pauseSnowAngle)->color = (gameState.pauseMenuSelection == 3) ? menuColorSelected : menuColorUnselected;
                registry.getUIText(pauseSnowBlur)->color = (gameState.pauseMenuSelection == 4) ? menuColorSelected : menuColorUnselected;
                registry.getUIText(pauseToonToggle)->color = (gameState.pauseMenuSelection == 5) ? menuColorSelected : menuColorUnselected;
                registry.getUIText(pauseMenuOption)->color = (gameState.pauseMenuSelection == 6) ? menuColorSelected : menuColorUnselected;
                uiSystem.clearCache();
            }

            // Handle left/right for speed, angle, and blur adjustment
            if (input.leftPressed || input.rightPressed) {
                float delta = input.rightPressed ? 1.0f : -1.0f;
                if (gameState.pauseMenuSelection == 2) {
                    // Adjust snow speed (0.1 to 10.0)
                    gameState.snowSpeed = glm::clamp(gameState.snowSpeed + delta * 0.5f, 0.1f, 10.0f);
                    char buf[48];
                    snprintf(buf, sizeof(buf), "SNOW SPEED: %.1f  < >", gameState.snowSpeed);
                    registry.getUIText(pauseSnowSpeed)->text = buf;
                    uiSystem.clearCache();
                }
                else if (gameState.pauseMenuSelection == 3) {
                    // Adjust snow angle (-180 to 180)
                    gameState.snowAngle = gameState.snowAngle + delta * 10.0f;
                    if (gameState.snowAngle > 180.0f) gameState.snowAngle -= 360.0f;
                    if (gameState.snowAngle < -180.0f) gameState.snowAngle += 360.0f;
                    char buf[48];
                    snprintf(buf, sizeof(buf), "SNOW ANGLE: %.0f  < >", gameState.snowAngle);
                    registry.getUIText(pauseSnowAngle)->text = buf;
                    uiSystem.clearCache();
                }
                else if (gameState.pauseMenuSelection == 4) {
                    // Adjust snow motion blur (0.0 to 5.0)
                    gameState.snowMotionBlur = glm::clamp(gameState.snowMotionBlur + delta * 0.5f, 0.0f, 5.0f);
                    char buf[48];
                    snprintf(buf, sizeof(buf), "SNOW BLUR: %.1f  < >", gameState.snowMotionBlur);
                    registry.getUIText(pauseSnowBlur)->text = buf;
                    uiSystem.clearCache();
                }
            }

            // Handle enter
            if (input.enterPressed) {
                if (gameState.pauseMenuSelection == 0) {
                    // Toggle fog
                    gameState.fogEnabled = !gameState.fogEnabled;
                    registry.getUIText(pauseFogToggle)->text = gameState.fogEnabled ? "FOG: YES" : "FOG: NO";
                    uiSystem.clearCache();
                }
                else if (gameState.pauseMenuSelection == 1) {
                    // Toggle snow
                    gameState.snowEnabled = !gameState.snowEnabled;
                    registry.getUIText(pauseSnowToggle)->text = gameState.snowEnabled ? "SNOW: YES" : "SNOW: NO";
                    uiSystem.clearCache();
                }
                else if (gameState.pauseMenuSelection == 5) {
                    // Toggle toon/comic mode
                    gameState.toonShadingEnabled = !gameState.toonShadingEnabled;
                    registry.getUIText(pauseToonToggle)->text = gameState.toonShadingEnabled ? "COMIC MODE: YES" : "COMIC MODE: NO";
                    uiSystem.clearCache();
                }
                else if (gameState.pauseMenuSelection == 6) {
                    // Back to main menu
                    sceneManager.switchTo(SceneType::MainMenu);
                }
            }

            // Clear with black for menu
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Only render UI
            uiSystem.update(registry, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
        }

        // === MSAA RESOLVE + BLIT (final pass for 3D scenes) ===
        // Resolve MSAA to regular texture, then blit to screen
        if (currentScene == SceneType::IntroCinematic ||
            currentScene == SceneType::PlayGame ||
            currentScene == SceneType::GodMode) {
            // Step 1: Resolve MSAA FBO to regular texture FBO
            glBindFramebuffer(GL_READ_FRAMEBUFFER, msaaFBO);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, resolveFBO);
            glBlitFramebuffer(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT,
                              0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT,
                              GL_COLOR_BUFFER_BIT, GL_LINEAR);

            // Step 2: Blit resolved texture to screen
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, GameConfig::WINDOW_WIDTH, GameConfig::WINDOW_HEIGHT);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);

            blitShader.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, resolveColorTex);
            blitShader.setInt("uScreenTexture", 0);

            glBindVertexArray(overlayVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindVertexArray(0);

            glEnable(GL_DEPTH_TEST);
        }

        windowManager.swapBuffers();
    }

    // Cleanup (WindowManager handles SDL/GL cleanup automatically)
    uiSystem.cleanup();
    axes.cleanup();

    return 0;
}
