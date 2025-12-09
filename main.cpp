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

    // Generate procedural buildings data (100x100 grid = 10,000 buildings)
    std::vector<BuildingGenerator::BuildingData> buildingDataList = BuildingGenerator::generateBuildingGrid(12345);
    Mesh buildingBoxMesh = BuildingGenerator::createUnitBoxMesh();  // Shared mesh for all buildings
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
    fogToggleText.offset = glm::vec2(0.0f, -30.0f);
    fogToggleText.horizontalAlign = HorizontalAlign::Center;
    fogToggleText.color = glm::vec4(255.0f, 255.0f, 255.0f, 255.0f);
    fogToggleText.visible = false;
    registry.addUIText(pauseFogToggle, fogToggleText);

    Entity pauseMenuOption = registry.create();
    UIText pauseMenuText;
    pauseMenuText.text = "BACK TO MAIN MENU";
    pauseMenuText.fontId = "oxanium_large";
    pauseMenuText.fontSize = 48;
    pauseMenuText.anchor = AnchorPoint::Center;
    pauseMenuText.offset = glm::vec2(0.0f, 30.0f);
    pauseMenuText.horizontalAlign = HorizontalAlign::Center;
    pauseMenuText.color = glm::vec4(128.0f, 128.0f, 128.0f, 255.0f);
    pauseMenuText.visible = false;
    registry.addUIText(pauseMenuOption, pauseMenuText);

    // Ground plane shader (uses model shader)
    Shader groundShader;
    groundShader.loadFromFiles("shaders/model.vert", "shaders/model.frag");

    // Debug axes
    Shader colorShader;
    colorShader.loadFromFiles("shaders/color.vert", "shaders/color.frag");
    AxisRenderer axes;
    axes.init();

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

    // Game loop
    bool running = true;
    while (running) {
        uint64_t currentTime = SDL_GetPerformanceCounter();
        float dt = (float)(currentTime - prevTime) / frequency;
        prevTime = currentTime;

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
            registry.getUIText(pauseMenuOption)->visible = false;

            if (scene == SceneType::MainMenu) {
                inputSystem.captureMouse(false);
                registry.getUIText(menuOption1)->visible = true;
                registry.getUIText(menuOption2)->visible = true;
                uiSystem.clearCache();  // Clear cache to update colors
            }
            else if (scene == SceneType::PlayGame) {
                inputSystem.captureMouse(true);
                registry.getUIText(sprintHint)->visible = true;

                // Only reset protagonist position when coming from main menu
                if (sceneManager.previous() == SceneType::MainMenu) {
                    auto* pt = registry.getTransform(protagonist);
                    if (pt) {
                        pt->position = glm::vec3(0.0f, 0.25f, 0.0f);
                    }
                    auto* pf = registry.getFacingDirection(protagonist);
                    if (pf) {
                        pf->yaw = 0.0f;
                    }
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
                registry.getUIText(pauseMenuOption)->visible = true;
                // Update colors based on selection
                registry.getUIText(pauseFogToggle)->color = menuColorSelected;
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
                    sceneManager.switchTo(SceneType::PlayGame);
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

            // Render
            glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Draw debug axes and compute view matrix
            auto* cam = registry.getCamera(camera);
            auto* camT = registry.getTransform(camera);
            auto* protagonistFacing = registry.getFacingDirection(protagonist);
            auto* ft = registry.getFollowTarget(camera);
            glm::mat4 playView;
            if (cam && camT && protagonistT && protagonistFacing && ft) {
                glm::vec3 lookAtPos = FollowCameraSystem::getLookAtPosition(*protagonistT, *ft, protagonistFacing->yaw);
                playView = glm::lookAt(camT->position, lookAtPos, glm::vec3(0.0f, 1.0f, 0.0f));
                glm::mat4 vp = cam->projectionMatrix(aspectRatio) * playView;
                colorShader.use();
                colorShader.setMat4("uMVP", vp);
                axes.draw();
            }

            renderSystem.setFogEnabled(fogEnabled);
            renderSystem.update(registry, aspectRatio);

            // Render ground plane
            glm::mat4 projection = cam->projectionMatrix(aspectRatio);
            glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
            groundShader.use();
            groundShader.setMat4("uView", playView);
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
            glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
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
            if (input.upPressed || input.downPressed) {
                pauseMenuSelection = 1 - pauseMenuSelection;  // Toggle between 0 and 1

                // Update menu colors
                auto* fogText = registry.getUIText(pauseFogToggle);
                auto* backText = registry.getUIText(pauseMenuOption);
                if (fogText && backText) {
                    fogText->color = (pauseMenuSelection == 0) ? menuColorSelected : menuColorUnselected;
                    backText->color = (pauseMenuSelection == 1) ? menuColorSelected : menuColorUnselected;
                    uiSystem.clearCache();
                }
            }

            // Handle enter
            if (input.enterPressed) {
                if (pauseMenuSelection == 0) {
                    // Toggle fog
                    fogEnabled = !fogEnabled;
                    auto* fogText = registry.getUIText(pauseFogToggle);
                    if (fogText) {
                        fogText->text = fogEnabled ? "FOG: YES" : "FOG: NO";
                        uiSystem.clearCache();
                    }
                } else {
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
