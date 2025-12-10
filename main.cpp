// fing-eternauta - ECS-based GLB Model Loader with Skeletal Animation
// Controls: WASD move character, Mouse rotate view, ESC exit

#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <stb_image.h>
#include <iostream>
#include <vector>

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
#include "src/procedural/BuildingGenerator.h"

int main(int argc, char* argv[]) {
    // SDL3 init
    if (!SDL_Init(SDL_INIT_VIDEO)) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return -1;
    }

    // SDL_ttf init
    if (!TTF_Init()) {
        std::cerr << "TTF_Init failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);

    const int WINDOW_WIDTH = 1280;
    const int WINDOW_HEIGHT = 720;

    SDL_Window* window = SDL_CreateWindow(
        "fing-eternauta",
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL
    );
    if (!window) {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext) {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress)) {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        SDL_GL_DestroyContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    std::cout << "OpenGL " << glGetString(GL_VERSION) << std::endl;

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);

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

    inputSystem.setWindow(window);

    // Load assets
    LoadedModel protagonistData = loadGLB("assets/protagonist.glb");

    // Create protagonist entity
    Entity protagonist = registry.create();
    Transform protagonistTransform;
    protagonistTransform.position = glm::vec3(0.0f, 0.0f, 0.0f);
    protagonistTransform.scale = glm::vec3(0.01f);
    registry.addTransform(protagonist, protagonistTransform);
    registry.addMeshGroup(protagonist, std::move(protagonistData.meshGroup));
    Renderable protagonistRenderable;
    protagonistRenderable.shader = ShaderType::Skinned;
    protagonistRenderable.meshOffset = glm::vec3(0.0f, -25.0f, 0.0f);  // Lower mesh so feet touch ground
    registry.addRenderable(protagonist, protagonistRenderable);

    // Add player controller
    PlayerController playerController;
    playerController.moveSpeed = 3.0f;
    playerController.turnSpeed = 10.0f;
    registry.addPlayerController(protagonist, playerController);

    // Add facing direction (decoupled from camera)
    FacingDirection facingDir;
    facingDir.yaw = 0.0f;
    facingDir.turnSpeed = 10.0f;
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

    Entity fingBuilding = registry.create();
    Transform fingTransform;
    // Move fing building outside the procedural grid (grid spans roughly -56 to +56)
    fingTransform.position = glm::vec3(80.0f, 10.0f, 80.0f);  // Outside grid, raised high
    fingTransform.rotation = glm::angleAxis(glm::radians(-90.0f), glm::vec3(1.0f, 0.0f, 0.0f));  // Rotate to stand upright
    fingTransform.scale = glm::vec3(2.5f);  // 2x larger (was 1.25)
    registry.addTransform(fingBuilding, fingTransform);
    registry.addMeshGroup(fingBuilding, MeshGroup{fingLowDetail.meshes});  // Start with LOD (far away)
    Renderable fingRenderable;
    fingRenderable.shader = ShaderType::Model;  // Non-animated model
    registry.addRenderable(fingBuilding, fingRenderable);

    // LOD settings
    const float lodSwitchDistance = 70.0f;  // Distance to switch between high/low detail
    bool fingUsingHighDetail = false;  // Track current LOD state

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

    // Generate procedural buildings data (100x100 grid = 10,000 buildings)
    std::vector<BuildingGenerator::BuildingData> buildingDataList = BuildingGenerator::generateBuildingGrid(12345);
    Mesh buildingBoxMesh = BuildingGenerator::createUnitBoxMesh();  // Shared mesh for all buildings
    buildingBoxMesh.texture = brickTexture;  // Apply brick texture to buildings
    std::cout << "Generated building data for " << buildingDataList.size() << " buildings" << std::endl;

    // Culling parameters: only render buildings within this radius of player's grid cell
    const int BUILDING_RENDER_RADIUS = 3;  // 3x3 = 7x7 grid around player (49 buildings max)
    const int MAX_VISIBLE_BUILDINGS = (2 * BUILDING_RENDER_RADIUS + 1) * (2 * BUILDING_RENDER_RADIUS + 1);

    // Create a pool of building entities that will be reused
    std::vector<Entity> buildingEntityPool;
    buildingEntityPool.reserve(MAX_VISIBLE_BUILDINGS);

    for (int i = 0; i < MAX_VISIBLE_BUILDINGS; ++i) {
        Entity buildingEntity = registry.create();

        Transform buildingTransform;
        buildingTransform.position = glm::vec3(0.0f, -1000.0f, 0.0f);  // Start hidden below ground
        buildingTransform.scale = glm::vec3(1.0f);
        registry.addTransform(buildingEntity, buildingTransform);

        MeshGroup buildingMeshGroup;
        buildingMeshGroup.meshes.push_back(buildingBoxMesh);
        registry.addMeshGroup(buildingEntity, std::move(buildingMeshGroup));

        Renderable buildingRenderable;
        buildingRenderable.shader = ShaderType::Model;
        buildingRenderable.triplanarMapping = true;  // Use world-space UV projection
        buildingRenderable.textureScale = 4.0f;      // Texture repeats every 4 world units
        registry.addRenderable(buildingEntity, buildingRenderable);

        // Add box collider for collision detection
        // halfExtents are (0.5, 0.5, 0.5) for unit box - will be scaled by transform.scale
        BoxCollider buildingCollider;
        buildingCollider.halfExtents = glm::vec3(0.5f);  // Unit box half-extents
        buildingCollider.offset = glm::vec3(0.0f);
        registry.addBoxCollider(buildingEntity, buildingCollider);

        buildingEntityPool.push_back(buildingEntity);
    }
    std::cout << "Created building entity pool with " << buildingEntityPool.size() << " entities" << std::endl;

    // Track which buildings are currently visible (for culling updates)
    int lastPlayerGridX = -9999;
    int lastPlayerGridZ = -9999;

    // Get building footprints for minimap (all buildings for now - could optimize later)
    auto buildingFootprints = BuildingGenerator::getBuildingFootprints(buildingDataList);

    // Create ground plane
    const float planeSize = 500.0f;
    const float texScale = 0.5f;  // Same as terrain - tiles every 2 units
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
    camComponent.fov = 60.0f;
    camComponent.nearPlane = 0.1f;
    camComponent.farPlane = 100.0f;
    camComponent.active = true;
    registry.addCamera(camera, camComponent);

    // Follow target links camera to protagonist (over-shoulder view)
    FollowTarget followTarget;
    followTarget.target = protagonist;
    registry.addFollowTarget(camera, followTarget);

    // Set up intro cinematic camera path
    // Path: Start in front of character, sweep around to behind (follow camera position)
    // Character faces FING building at (80, 10, 80) - yaw ~225 degrees
    const float characterYaw = 225.0f;  // Face toward FING (positive X, positive Z diagonal)
    const glm::vec3 characterPos(0.0f, 0.1f, 0.0f);  // Protagonist position during cinematic

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
        glm::vec3 camStart(6.0f, 3.0f, 6.0f);  // Start: in front of character (toward FING direction)
        introPath.addControlPoint(camStart);
        introPath.addControlPoint(glm::vec3(3.0f, 2.5f, 0.0f));    // Sweep to the side
        introPath.addControlPoint(glm::vec3(-2.0f, 2.0f, -2.0f));  // Moving toward final position
        // Add intermediate point close to final to ensure smooth approach (avoid spline snap)
        glm::vec3 nearFinal = glm::mix(glm::vec3(-2.0f, 2.0f, -2.0f), followCamEndPos, 0.7f);
        introPath.addControlPoint(nearFinal);
        introPath.addControlPoint(followCamEndPos);                 // End: behind character (follow camera)

        cinematicSystem.setCameraPath(introPath);
        cinematicSystem.setLookAtTarget(protagonist);
        cinematicSystem.setFinalLookAt(followCamLookAt);  // Blend to gameplay look-at at end
        cinematicSystem.setDuration(3.0f);  // 3 seconds

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
    fogToggleText.text = "FOG: NO";
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

    Entity pauseMenuOption = registry.create();
    UIText pauseMenuText;
    pauseMenuText.text = "BACK TO MAIN MENU";
    pauseMenuText.fontId = "oxanium_large";
    pauseMenuText.fontSize = 48;
    pauseMenuText.anchor = AnchorPoint::Center;
    pauseMenuText.offset = glm::vec2(0.0f, 210.0f);
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
    introHeaderText.offset = glm::vec2(730.0f, 80.0f);  // Positioned to end near right edge when complete
    introHeaderText.horizontalAlign = HorizontalAlign::Left;
    introHeaderText.color = introTextColor;
    introHeaderText.visible = false;
    registry.addUIText(introHeader, introHeaderText);
    introTextEntities.push_back(introHeader);

    // Body paragraphs: left-aligned story text, centered on screen
    const float introLeftMargin = 45.0f;  // Adjusted margin to center text better
    const float introLineHeight = 100.0f;
    const float introStartY = 180.0f;

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
    int introCurrentLine = 0;
    size_t introCurrentChar = 0;
    float introTypewriterTimer = 0.0f;
    const float introCharDelay = 0.08f;
    const float introLineDelay = 0.5f;
    float introLinePauseTimer = 0.0f;
    bool introLineComplete = false;
    bool introAllComplete = false;

    // Ground plane shader (uses model shader)
    Shader groundShader;
    groundShader.loadFromFiles("shaders/model.vert", "shaders/model.frag");

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

    // Shadow mapping setup
    Shader depthShader;
    depthShader.loadFromFiles("shaders/depth.vert", "shaders/depth.frag");
    const int SHADOW_WIDTH = 2048, SHADOW_HEIGHT = 2048;
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
    Shader motionBlurShader;
    motionBlurShader.loadFromFiles("shaders/motion_blur.vert", "shaders/motion_blur.frag");
    GLuint motionBlurFBO[2];      // Two FBOs for ping-pong
    GLuint motionBlurColorTex[2]; // Color attachments
    GLuint motionBlurDepthRBO;    // Shared depth buffer
    bool motionBlurInitialized = false;  // Track if we need to clear accumulation
    int motionBlurPingPong = 0;   // Current read buffer (write to the other)
    {
        glGenFramebuffers(2, motionBlurFBO);
        glGenTextures(2, motionBlurColorTex);
        glGenRenderbuffers(1, &motionBlurDepthRBO);

        for (int i = 0; i < 2; ++i) {
            glBindTexture(GL_TEXTURE_2D, motionBlurColorTex[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, WINDOW_WIDTH, WINDOW_HEIGHT, 0, GL_RGB, GL_FLOAT, NULL);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glBindFramebuffer(GL_FRAMEBUFFER, motionBlurFBO[i]);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, motionBlurColorTex[i], 0);
        }

        // Shared depth renderbuffer (for scene rendering)
        glBindRenderbuffer(GL_RENDERBUFFER, motionBlurDepthRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, WINDOW_WIDTH, WINDOW_HEIGHT);

        // Attach depth to both FBOs
        for (int i = 0; i < 2; ++i) {
            glBindFramebuffer(GL_FRAMEBUFFER, motionBlurFBO[i]);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, motionBlurDepthRBO);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
    }
    const float cinematicMotionBlurStrength = 0.85f;  // Strong but not overwhelming blur

    // Light direction (same as in shaders)
    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));

    // Shadertoy overlay (fullscreen snow effect)
    Shader overlayShader;
    overlayShader.loadFromFiles("shaders/shadertoy_overlay.vert", "shaders/shadertoy_overlay.frag");
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
    float gameTime = 0.0f;  // Track time for shader effects

    // Timing
    uint64_t prevTime = SDL_GetPerformanceCounter();
    uint64_t frequency = SDL_GetPerformanceFrequency();
    float aspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

    // Menu state
    int menuSelection = 0;  // 0 = Play Game, 1 = God Mode
    int pauseMenuSelection = 0;  // 0 = Fog toggle, 1 = Back to main menu
    const glm::vec4 menuColorSelected(255.0f, 255.0f, 255.0f, 255.0f);
    const glm::vec4 menuColorUnselected(128.0f, 128.0f, 128.0f, 255.0f);

    // Game settings
    bool fogEnabled = false;
    bool snowEnabled = true;
    float snowSpeed = 7.0f;
    float snowAngle = 20.0f;
    float snowMotionBlur = 3.0f;  // 0.0 = no blur, higher = more trail
    const int PAUSE_MENU_ITEMS = 6;  // Fog, Snow, Snow Speed, Snow Angle, Snow Blur, Back

    // Game loop
    bool running = true;
    while (running) {
        uint64_t currentTime = SDL_GetPerformanceCounter();
        float dt = (float)(currentTime - prevTime) / frequency;
        prevTime = currentTime;
        gameTime += dt;  // Accumulate time for shader effects

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

                // Reset protagonist position (slightly above ground)
                auto* pt = registry.getTransform(protagonist);
                if (pt) {
                    pt->position = glm::vec3(0.0f, 0.1f, 0.0f);
                }
                // Character faces toward FING (yaw 225 = toward +X,+Z diagonal)
                auto* pf = registry.getFacingDirection(protagonist);
                if (pf) {
                    pf->yaw = 225.0f;
                }

                // Reset motion blur accumulation for fresh start
                motionBlurInitialized = false;
                motionBlurPingPong = 0;

                // Start the cinematic
                cinematicSystem.start(registry);
            }
            else if (scene == SceneType::PlayGame) {
                inputSystem.captureMouse(true);
                registry.getUIText(sprintHint)->visible = true;

                // Reset protagonist position when starting gameplay (from intro cinematic or direct)
                auto* pt = registry.getTransform(protagonist);
                if (pt) {
                    pt->position = glm::vec3(0.0f, 0.1f, 0.0f);  // Slightly above ground to avoid clipping
                }
                auto* pf = registry.getFacingDirection(protagonist);
                if (pf) {
                    pf->yaw = 225.0f;  // Face toward FING (+X,+Z diagonal)
                }
            }
            else if (scene == SceneType::GodMode) {
                inputSystem.captureMouse(true);
                registry.getUIText(godModeHint)->visible = true;

                // Set camera to a good viewing position
                auto* ct = registry.getTransform(camera);
                if (ct) {
                    ct->position = glm::vec3(5.0f, 3.0f, 5.0f);
                }
                freeCameraSystem.setPosition(glm::vec3(5.0f, 3.0f, 5.0f), -45.0f, -15.0f);
            }
            else if (scene == SceneType::PauseMenu) {
                inputSystem.captureMouse(false);
                pauseMenuSelection = 0;  // Reset to first option
                registry.getUIText(pauseFogToggle)->visible = true;
                registry.getUIText(pauseSnowToggle)->visible = true;
                registry.getUIText(pauseSnowSpeed)->visible = true;
                registry.getUIText(pauseSnowAngle)->visible = true;
                registry.getUIText(pauseSnowBlur)->visible = true;
                registry.getUIText(pauseMenuOption)->visible = true;
                // Update colors based on selection
                registry.getUIText(pauseFogToggle)->color = menuColorSelected;
                registry.getUIText(pauseSnowToggle)->color = menuColorUnselected;
                registry.getUIText(pauseSnowSpeed)->color = menuColorUnselected;
                registry.getUIText(pauseSnowAngle)->color = menuColorUnselected;
                registry.getUIText(pauseSnowBlur)->color = menuColorUnselected;
                registry.getUIText(pauseMenuOption)->color = menuColorUnselected;
                uiSystem.clearCache();
            }
        }

        SceneType currentScene = sceneManager.current();

        // Scene-specific update logic
        if (currentScene == SceneType::MainMenu) {
            // Menu navigation
            if (input.upPressed || input.downPressed) {
                menuSelection = 1 - menuSelection;  // Toggle between 0 and 1

                // Update menu colors
                auto* text1 = registry.getUIText(menuOption1);
                auto* text2 = registry.getUIText(menuOption2);
                if (text1 && text2) {
                    text1->color = (menuSelection == 0) ? menuColorSelected : menuColorUnselected;
                    text2->color = (menuSelection == 1) ? menuColorSelected : menuColorUnselected;
                    uiSystem.clearCache();  // Force re-render with new colors
                }
            }

            if (input.enterPressed) {
                if (menuSelection == 0) {
                    sceneManager.switchTo(SceneType::IntroText);
                } else {
                    sceneManager.switchTo(SceneType::GodMode);
                }
            }

            // Clear with black for menu
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Only render UI
            uiSystem.update(registry, WINDOW_WIDTH, WINDOW_HEIGHT);
        }
        else if (currentScene == SceneType::IntroText) {
            // Skip intro with Enter or Escape
            if (input.enterPressed || input.escapePressed) {
                sceneManager.switchTo(SceneType::IntroCinematic);
            }

            // Typewriter effect update
            if (!introAllComplete) {
                if (introLineComplete) {
                    // Waiting between lines
                    introLinePauseTimer += dt;
                    if (introLinePauseTimer >= introLineDelay) {
                        introLinePauseTimer = 0.0f;
                        introLineComplete = false;
                        introCurrentLine++;
                        introCurrentChar = 0;
                        if (introCurrentLine >= (int)introTexts.size()) {
                            introAllComplete = true;
                        }
                    }
                } else if (introCurrentLine < (int)introTexts.size()) {
                    // Typing characters
                    introTypewriterTimer += dt;
                    while (introTypewriterTimer >= introCharDelay && introCurrentChar < introTexts[introCurrentLine].length()) {
                        introTypewriterTimer -= introCharDelay;
                        introCurrentChar++;
                        // Update the text entity
                        auto* text = registry.getUIText(introTextEntities[introCurrentLine]);
                        if (text) {
                            text->text = introTexts[introCurrentLine].substr(0, introCurrentChar);
                            uiSystem.clearCache();
                        }
                    }
                    // Check if line is complete
                    if (introCurrentChar >= introTexts[introCurrentLine].length()) {
                        introLineComplete = true;
                        introLinePauseTimer = 0.0f;
                    }
                }
            } else {
                // All text complete, wait a moment then transition
                introLinePauseTimer += dt;
                if (introLinePauseTimer >= 2.0f) {  // 2 second pause after all text
                    sceneManager.switchTo(SceneType::IntroCinematic);
                }
            }

            // Clear with black for intro text
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Only render UI
            uiSystem.update(registry, WINDOW_WIDTH, WINDOW_HEIGHT);
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

            // Update building culling for cinematic (character at origin)
            auto* protagonistT = registry.getTransform(protagonist);
            if (protagonistT) {
                auto [playerGridX, playerGridZ] = BuildingGenerator::getPlayerGridCell(protagonistT->position);

                if (playerGridX != lastPlayerGridX || playerGridZ != lastPlayerGridZ) {
                    lastPlayerGridX = playerGridX;
                    lastPlayerGridZ = playerGridZ;

                    std::vector<const BuildingGenerator::BuildingData*> visibleBuildings;
                    for (const auto& bldg : buildingDataList) {
                        if (BuildingGenerator::isBuildingInRange(bldg, playerGridX, playerGridZ, BUILDING_RENDER_RADIUS)) {
                            visibleBuildings.push_back(&bldg);
                        }
                    }

                    size_t entityIdx = 0;
                    for (const auto* bldg : visibleBuildings) {
                        if (entityIdx >= buildingEntityPool.size()) break;
                        auto* transform = registry.getTransform(buildingEntityPool[entityIdx]);
                        if (transform) {
                            transform->position = bldg->position;
                            transform->scale = glm::vec3(bldg->width, bldg->height, bldg->depth);
                        }
                        ++entityIdx;
                    }
                    for (; entityIdx < buildingEntityPool.size(); ++entityIdx) {
                        auto* transform = registry.getTransform(buildingEntityPool[entityIdx]);
                        if (transform) {
                            transform->position.y = -1000.0f;
                        }
                    }
                }
            }

            // Get cinematic view matrix
            auto* cam = registry.getCamera(camera);
            auto* camT = registry.getTransform(camera);
            glm::mat4 cinematicView = cinematicSystem.getViewMatrix(registry);
            glm::mat4 projection = cam ? cam->projectionMatrix(aspectRatio) : glm::mat4(1.0f);

            // === SHADOW PASS (cinematic) ===
            float orthoSize = 100.0f;
            glm::vec3 lightPos = (protagonistT ? protagonistT->position : glm::vec3(0.0f)) + lightDir * 80.0f;
            glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, 200.0f);
            glm::mat4 lightView = glm::lookAt(lightPos, protagonistT ? protagonistT->position : glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 lightSpaceMatrix = lightProjection * lightView;

            glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
            glClear(GL_DEPTH_BUFFER_BIT);

            depthShader.use();
            depthShader.setMat4("uLightSpaceMatrix", lightSpaceMatrix);

            // Render buildings to shadow map
            for (Entity e : buildingEntityPool) {
                auto* t = registry.getTransform(e);
                if (t && t->position.y > -100.0f) {
                    depthShader.setMat4("uModel", t->matrix());
                    auto* mg = registry.getMeshGroup(e);
                    if (mg) {
                        for (const auto& mesh : mg->meshes) {
                            glBindVertexArray(mesh.vao);
                            glDrawElements(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr);
                        }
                    }
                }
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

            // === RENDER CINEMATIC SCENE TO FBO FOR MOTION BLUR ===
            // Determine which FBO to render to (the one that's NOT the previous frame)
            int writeIdx = 1 - motionBlurPingPong;
            glBindFramebuffer(GL_FRAMEBUFFER, motionBlurFBO[writeIdx]);
            glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

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
            renderSystem.setFogEnabled(fogEnabled);
            renderSystem.setShadowsEnabled(true);
            renderSystem.setShadowMap(shadowDepthTexture);
            renderSystem.setLightSpaceMatrix(lightSpaceMatrix);
            renderSystem.updateWithView(registry, aspectRatio, cinematicView);

            // Render ground plane with shadows
            groundShader.use();
            groundShader.setMat4("uView", cinematicView);
            groundShader.setMat4("uProjection", projection);
            groundShader.setMat4("uModel", glm::mat4(1.0f));
            groundShader.setVec3("uLightDir", lightDir);
            groundShader.setVec3("uViewPos", cinematicSystem.getCurrentCameraPosition());
            groundShader.setInt("uHasTexture", 1);
            groundShader.setInt("uFogEnabled", fogEnabled ? 1 : 0);
            groundShader.setInt("uShadowsEnabled", 1);
            groundShader.setMat4("uLightSpaceMatrix", lightSpaceMatrix);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, snowTexture);
            groundShader.setInt("uTexture", 0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);
            groundShader.setInt("uShadowMap", 1);
            glBindVertexArray(planeVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
            glBindVertexArray(0);

            // Render snow overlay to the FBO
            if (snowEnabled) {
                glDisable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                overlayShader.use();
                overlayShader.setVec3("iResolution", glm::vec3((float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, 1.0f));
                overlayShader.setFloat("iTime", gameTime);
                overlayShader.setFloat("uSnowSpeed", snowSpeed);
                overlayShader.setFloat("uSnowDirectionDeg", snowAngle);
                overlayShader.setFloat("uMotionBlur", snowMotionBlur);

                glBindVertexArray(overlayVAO);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBindVertexArray(0);

                glDisable(GL_BLEND);
                glEnable(GL_DEPTH_TEST);
            }

            // === MOTION BLUR POST-PROCESS ===
            // Step 1: Blend current frame with previous accumulated frame, write to the OTHER FBO
            int readIdx = motionBlurPingPong;  // Previous accumulated frame
            int accumIdx = writeIdx;            // Where we just rendered current frame

            // We need a third pass: blend into readIdx (the old accumulation buffer)
            // Actually simpler: just blend and display, but ALSO write to accumulation

            // First, copy the blended result to screen
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glDisable(GL_DEPTH_TEST);

            motionBlurShader.use();
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, motionBlurColorTex[accumIdx]);  // Current frame we just rendered
            motionBlurShader.setInt("uCurrentFrame", 0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, motionBlurColorTex[readIdx]);  // Previous accumulated frame
            motionBlurShader.setInt("uPreviousFrame", 1);

            // On first frame, don't blend with previous (it's garbage)
            float blendFactor = motionBlurInitialized ? cinematicMotionBlurStrength : 0.0f;
            motionBlurShader.setFloat("uBlendFactor", blendFactor);

            glBindVertexArray(overlayVAO);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);

            // Step 2: Also write the blended result back to accumIdx FBO for next frame
            glBindFramebuffer(GL_FRAMEBUFFER, motionBlurFBO[accumIdx]);
            glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);

            glBindVertexArray(0);
            glEnable(GL_DEPTH_TEST);

            // Swap ping-pong buffers and mark as initialized
            motionBlurPingPong = accumIdx;
            motionBlurInitialized = true;
        }
        else if (currentScene == SceneType::PlayGame) {
            // Open pause menu on escape
            if (input.escapePressed) {
                sceneManager.switchTo(SceneType::PauseMenu);
            }

            // Update gameplay systems
            cameraOrbitSystem.update(registry, input.mouseX, input.mouseY);
            playerMovementSystem.update(registry, dt);
            followCameraSystem.update(registry);
            physicsSystem.update(registry, dt);
            collisionSystem.update(registry);
            animationSystem.update(registry, dt);
            skeletonSystem.update(registry);

            // LOD update for fing building
            auto* protagonistT = registry.getTransform(protagonist);
            auto* fingBuildingT = registry.getTransform(fingBuilding);
            if (protagonistT && fingBuildingT) {
                float distToBuilding = glm::length(protagonistT->position - fingBuildingT->position);
                bool shouldUseHighDetail = distToBuilding < lodSwitchDistance;

                if (shouldUseHighDetail != fingUsingHighDetail) {
                    fingUsingHighDetail = shouldUseHighDetail;
                    auto* meshGroup = registry.getMeshGroup(fingBuilding);
                    if (meshGroup) {
                        meshGroup->meshes = fingUsingHighDetail ? fingHighDetail.meshes : fingLowDetail.meshes;
                    }
                }
            }

            // Building culling: update visible buildings based on player position
            if (protagonistT) {
                auto [playerGridX, playerGridZ] = BuildingGenerator::getPlayerGridCell(protagonistT->position);

                // Only update if player moved to a different grid cell
                if (playerGridX != lastPlayerGridX || playerGridZ != lastPlayerGridZ) {
                    lastPlayerGridX = playerGridX;
                    lastPlayerGridZ = playerGridZ;

                    // Collect buildings in range
                    std::vector<const BuildingGenerator::BuildingData*> visibleBuildings;
                    for (const auto& bldg : buildingDataList) {
                        if (BuildingGenerator::isBuildingInRange(bldg, playerGridX, playerGridZ, BUILDING_RENDER_RADIUS)) {
                            visibleBuildings.push_back(&bldg);
                        }
                    }

                    // Update entity pool with visible buildings
                    size_t entityIdx = 0;
                    for (const auto* bldg : visibleBuildings) {
                        if (entityIdx >= buildingEntityPool.size()) break;

                        auto* transform = registry.getTransform(buildingEntityPool[entityIdx]);
                        if (transform) {
                            transform->position = bldg->position;
                            transform->scale = glm::vec3(bldg->width, bldg->height, bldg->depth);
                        }
                        ++entityIdx;
                    }

                    // Hide remaining entities in the pool
                    for (; entityIdx < buildingEntityPool.size(); ++entityIdx) {
                        auto* transform = registry.getTransform(buildingEntityPool[entityIdx]);
                        if (transform) {
                            transform->position.y = -1000.0f;  // Hide below ground
                        }
                    }
                }
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

            // === SHADOW PASS ===
            // Compute light space matrix centered on player
            float orthoSize = 100.0f;
            glm::vec3 lightPos = (protagonistT ? protagonistT->position : glm::vec3(0.0f)) + lightDir * 80.0f;
            glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, -orthoSize, orthoSize, 1.0f, 200.0f);
            glm::mat4 lightView = glm::lookAt(lightPos, protagonistT ? protagonistT->position : glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));
            glm::mat4 lightSpaceMatrix = lightProjection * lightView;

            glViewport(0, 0, SHADOW_WIDTH, SHADOW_HEIGHT);
            glBindFramebuffer(GL_FRAMEBUFFER, shadowFBO);
            glClear(GL_DEPTH_BUFFER_BIT);

            depthShader.use();
            depthShader.setMat4("uLightSpaceMatrix", lightSpaceMatrix);

            // Render buildings to shadow map
            for (Entity e : buildingEntityPool) {
                auto* t = registry.getTransform(e);
                if (t && t->position.y > -100.0f) {
                    depthShader.setMat4("uModel", t->matrix());
                    auto* mg = registry.getMeshGroup(e);
                    if (mg) {
                        for (const auto& mesh : mg->meshes) {
                            glBindVertexArray(mesh.vao);
                            glDrawElements(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr);
                        }
                    }
                }
            }

            // Render fing building to shadow map
            {
                auto* t = registry.getTransform(fingBuilding);
                auto* mg = registry.getMeshGroup(fingBuilding);
                if (t && mg) {
                    depthShader.setMat4("uModel", t->matrix());
                    for (const auto& mesh : mg->meshes) {
                        glBindVertexArray(mesh.vao);
                        glDrawElements(GL_TRIANGLES, mesh.indexCount, mesh.indexType, nullptr);
                    }
                }
            }

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

            // === MAIN RENDER PASS ===
            glClearColor(0.2f, 0.2f, 0.22f, 1.0f);  // Dark gray sky
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Draw debug axes
            if (cam && camT && protagonistT && protagonistFacing && ft) {
                glm::mat4 vp = projection * playView;
                colorShader.use();
                colorShader.setMat4("uMVP", vp);
                axes.draw();
            }

            // Set shadow uniforms for render system
            renderSystem.setFogEnabled(fogEnabled);
            renderSystem.setShadowsEnabled(true);
            renderSystem.setShadowMap(shadowDepthTexture);
            renderSystem.setLightSpaceMatrix(lightSpaceMatrix);
            renderSystem.update(registry, aspectRatio);

            // Render ground plane with shadows
            groundShader.use();
            groundShader.setMat4("uView", playView);
            groundShader.setMat4("uProjection", projection);
            groundShader.setMat4("uModel", glm::mat4(1.0f));
            groundShader.setMat4("uLightSpaceMatrix", lightSpaceMatrix);
            groundShader.setVec3("uLightDir", lightDir);
            groundShader.setVec3("uViewPos", camT->position);
            groundShader.setInt("uHasTexture", 1);
            groundShader.setInt("uFogEnabled", fogEnabled ? 1 : 0);
            groundShader.setInt("uShadowsEnabled", 1);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, snowTexture);
            groundShader.setInt("uTexture", 0);
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);
            groundShader.setInt("uShadowMap", 1);
            glBindVertexArray(planeVAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
            glBindVertexArray(0);

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

            // Render snow overlay (fullscreen 2D effect)
            if (snowEnabled) {
                glDisable(GL_DEPTH_TEST);
                glEnable(GL_BLEND);
                glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

                overlayShader.use();
                overlayShader.setVec3("iResolution", glm::vec3((float)WINDOW_WIDTH, (float)WINDOW_HEIGHT, 1.0f));
                overlayShader.setFloat("iTime", gameTime);
                overlayShader.setFloat("uSnowSpeed", snowSpeed);
                overlayShader.setFloat("uSnowDirectionDeg", snowAngle);
                overlayShader.setFloat("uMotionBlur", snowMotionBlur);

                glBindVertexArray(overlayVAO);
                glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
                glBindVertexArray(0);

                glDisable(GL_BLEND);
                glEnable(GL_DEPTH_TEST);
            }

            // Build marker positions for minimap
            std::vector<glm::vec3> minimapMarkers;
            if (fingBuildingT) {
                minimapMarkers.push_back(fingBuildingT->position);
            }
            minimapSystem.render(WINDOW_WIDTH, WINDOW_HEIGHT, protagonistFacing ? protagonistFacing->yaw : 0.0f,
                                 uiSystem.fonts(), uiSystem.textCache(),
                                 protagonistT ? protagonistT->position : glm::vec3(0.0f), minimapMarkers,
                                 buildingFootprints);
            uiSystem.update(registry, WINDOW_WIDTH, WINDOW_HEIGHT);
        }
        else if (currentScene == SceneType::GodMode) {
            // Open pause menu on escape
            if (input.escapePressed) {
                sceneManager.switchTo(SceneType::PauseMenu);
            }

            // Free camera control
            freeCameraSystem.update(registry, dt, input.mouseX, input.mouseY);

            // Still update animations for visual effect
            animationSystem.update(registry, dt);
            skeletonSystem.update(registry);

            // LOD update for fing building (based on camera distance in god mode)
            auto* camT_lod = registry.getTransform(camera);
            auto* fingBuildingT_lod = registry.getTransform(fingBuilding);
            if (camT_lod && fingBuildingT_lod) {
                float distToBuilding = glm::length(camT_lod->position - fingBuildingT_lod->position);
                bool shouldUseHighDetail = distToBuilding < lodSwitchDistance;

                if (shouldUseHighDetail != fingUsingHighDetail) {
                    fingUsingHighDetail = shouldUseHighDetail;
                    auto* meshGroup = registry.getMeshGroup(fingBuilding);
                    if (meshGroup) {
                        meshGroup->meshes = fingUsingHighDetail ? fingHighDetail.meshes : fingLowDetail.meshes;
                    }
                }
            }

            // Render
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

                renderSystem.setFogEnabled(fogEnabled);
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
                groundShader.setInt("uFogEnabled", fogEnabled ? 1 : 0);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, snowTexture);
                groundShader.setInt("uTexture", 0);
                glBindVertexArray(planeVAO);
                glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_SHORT, nullptr);
                glBindVertexArray(0);
            }
            minimapSystem.render(WINDOW_WIDTH, WINDOW_HEIGHT);
            uiSystem.update(registry, WINDOW_WIDTH, WINDOW_HEIGHT);
        }
        else if (currentScene == SceneType::PauseMenu) {
            // Resume game on escape
            if (input.escapePressed) {
                sceneManager.switchTo(sceneManager.previous());
            }

            // Menu navigation
            if (input.upPressed) {
                pauseMenuSelection = (pauseMenuSelection - 1 + PAUSE_MENU_ITEMS) % PAUSE_MENU_ITEMS;
            }
            if (input.downPressed) {
                pauseMenuSelection = (pauseMenuSelection + 1) % PAUSE_MENU_ITEMS;
            }

            // Update menu colors based on selection
            if (input.upPressed || input.downPressed) {
                registry.getUIText(pauseFogToggle)->color = (pauseMenuSelection == 0) ? menuColorSelected : menuColorUnselected;
                registry.getUIText(pauseSnowToggle)->color = (pauseMenuSelection == 1) ? menuColorSelected : menuColorUnselected;
                registry.getUIText(pauseSnowSpeed)->color = (pauseMenuSelection == 2) ? menuColorSelected : menuColorUnselected;
                registry.getUIText(pauseSnowAngle)->color = (pauseMenuSelection == 3) ? menuColorSelected : menuColorUnselected;
                registry.getUIText(pauseSnowBlur)->color = (pauseMenuSelection == 4) ? menuColorSelected : menuColorUnselected;
                registry.getUIText(pauseMenuOption)->color = (pauseMenuSelection == 5) ? menuColorSelected : menuColorUnselected;
                uiSystem.clearCache();
            }

            // Handle left/right for speed, angle, and blur adjustment
            if (input.leftPressed || input.rightPressed) {
                float delta = input.rightPressed ? 1.0f : -1.0f;
                if (pauseMenuSelection == 2) {
                    // Adjust snow speed (0.1 to 10.0)
                    snowSpeed = glm::clamp(snowSpeed + delta * 0.5f, 0.1f, 10.0f);
                    char buf[48];
                    snprintf(buf, sizeof(buf), "SNOW SPEED: %.1f  < >", snowSpeed);
                    registry.getUIText(pauseSnowSpeed)->text = buf;
                    uiSystem.clearCache();
                }
                else if (pauseMenuSelection == 3) {
                    // Adjust snow angle (-180 to 180)
                    snowAngle = snowAngle + delta * 10.0f;
                    if (snowAngle > 180.0f) snowAngle -= 360.0f;
                    if (snowAngle < -180.0f) snowAngle += 360.0f;
                    char buf[48];
                    snprintf(buf, sizeof(buf), "SNOW ANGLE: %.0f  < >", snowAngle);
                    registry.getUIText(pauseSnowAngle)->text = buf;
                    uiSystem.clearCache();
                }
                else if (pauseMenuSelection == 4) {
                    // Adjust snow motion blur (0.0 to 5.0)
                    snowMotionBlur = glm::clamp(snowMotionBlur + delta * 0.5f, 0.0f, 5.0f);
                    char buf[48];
                    snprintf(buf, sizeof(buf), "SNOW BLUR: %.1f  < >", snowMotionBlur);
                    registry.getUIText(pauseSnowBlur)->text = buf;
                    uiSystem.clearCache();
                }
            }

            // Handle enter
            if (input.enterPressed) {
                if (pauseMenuSelection == 0) {
                    // Toggle fog
                    fogEnabled = !fogEnabled;
                    registry.getUIText(pauseFogToggle)->text = fogEnabled ? "FOG: YES" : "FOG: NO";
                    uiSystem.clearCache();
                }
                else if (pauseMenuSelection == 1) {
                    // Toggle snow
                    snowEnabled = !snowEnabled;
                    registry.getUIText(pauseSnowToggle)->text = snowEnabled ? "SNOW: YES" : "SNOW: NO";
                    uiSystem.clearCache();
                }
                else if (pauseMenuSelection == 5) {
                    // Back to main menu
                    sceneManager.switchTo(SceneType::MainMenu);
                }
            }

            // Clear with black for menu
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Only render UI
            uiSystem.update(registry, WINDOW_WIDTH, WINDOW_HEIGHT);
        }

        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    uiSystem.cleanup();
    axes.cleanup();
    SDL_GL_DestroyContext(glContext);
    SDL_DestroyWindow(window);
    TTF_Quit();
    SDL_Quit();

    return 0;
}
