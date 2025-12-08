// fing-eternauta - ECS-based GLB Model Loader with Skeletal Animation
// Controls: WASD move character, Mouse rotate view, ESC exit

#include <glad/glad.h>
#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <iostream>

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
#include "src/assets/AssetLoader.h"
#include "src/DebugRenderer.h"
#include "src/Shader.h"
#include "src/scenes/SceneManager.h"

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

    // Load UI fonts
    if (!uiSystem.fonts().loadFont("oxanium", "assets/fonts/Oxanium.ttf", 28)) {
        std::cerr << "Failed to load Oxanium font" << std::endl;
    }
    if (!uiSystem.fonts().loadFont("oxanium_large", "assets/fonts/Oxanium.ttf", 48)) {
        std::cerr << "Failed to load Oxanium large font" << std::endl;
    }

    inputSystem.setWindow(window);

    // Load assets
    LoadedModel protagonistData = loadGLB("assets/protagonist.glb");

    // Create protagonist entity
    Entity protagonist = registry.create();
    Transform protagonistTransform;
    protagonistTransform.position = glm::vec3(0.0f, 0.25f, 0.0f);
    protagonistTransform.scale = glm::vec3(0.01f);
    registry.addTransform(protagonist, protagonistTransform);
    registry.addMeshGroup(protagonist, std::move(protagonistData.meshGroup));
    Renderable protagonistRenderable;
    protagonistRenderable.shader = ShaderType::Skinned;
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

    // Create ground plane
    const float planeSize = 500.0f;
    Entity ground = registry.create();
    Transform groundTransform;
    groundTransform.position = glm::vec3(0.0f, 0.0f, 0.0f);
    registry.addTransform(ground, groundTransform);

    // Create plane mesh (position, normal, uv)
    float planeVertices[] = {
        // Position              // Normal       // UV
        -planeSize, 0.0f, -planeSize,  0.0f, 1.0f, 0.0f,  0.0f, 0.0f,
         planeSize, 0.0f, -planeSize,  0.0f, 1.0f, 0.0f,  1.0f, 0.0f,
         planeSize, 0.0f,  planeSize,  0.0f, 1.0f, 0.0f,  1.0f, 1.0f,
        -planeSize, 0.0f,  planeSize,  0.0f, 1.0f, 0.0f,  0.0f, 1.0f,
    };
    unsigned short planeIndices[] = { 0, 1, 2, 0, 2, 3 };

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

    Mesh planeMesh;
    planeMesh.vao = planeVAO;
    planeMesh.indexCount = 6;
    planeMesh.indexType = GL_UNSIGNED_SHORT;
    planeMesh.hasSkinning = false;
    planeMesh.texture = 0;

    MeshGroup groundMeshGroup;
    groundMeshGroup.meshes.push_back(planeMesh);
    registry.addMeshGroup(ground, std::move(groundMeshGroup));

    Renderable groundRenderable;
    groundRenderable.shader = ShaderType::Model;
    registry.addRenderable(ground, groundRenderable);

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
    const glm::vec4 menuColorSelected(255.0f, 255.0f, 255.0f, 255.0f);
    const glm::vec4 menuColorUnselected(128.0f, 128.0f, 128.0f, 255.0f);

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

            if (scene == SceneType::MainMenu) {
                inputSystem.captureMouse(false);
                registry.getUIText(menuOption1)->visible = true;
                registry.getUIText(menuOption2)->visible = true;
                uiSystem.clearCache();  // Clear cache to update colors
            }
            else if (scene == SceneType::PlayGame) {
                inputSystem.captureMouse(true);
                registry.getUIText(sprintHint)->visible = true;

                // Reset protagonist position
                auto* pt = registry.getTransform(protagonist);
                if (pt) {
                    pt->position = glm::vec3(0.0f, 0.25f, 0.0f);
                }
                auto* pf = registry.getFacingDirection(protagonist);
                if (pf) {
                    pf->yaw = 0.0f;
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
            // Return to menu on escape
            if (input.escapePressed) {
                sceneManager.switchTo(SceneType::MainMenu);
            }

            // Update gameplay systems
            cameraOrbitSystem.update(registry, input.mouseX, input.mouseY);
            playerMovementSystem.update(registry, dt);
            followCameraSystem.update(registry);
            physicsSystem.update(registry, dt);
            collisionSystem.update(registry);
            animationSystem.update(registry, dt);
            skeletonSystem.update(registry);

            // Render
            glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // Draw debug axes
            auto* cam = registry.getCamera(camera);
            auto* camT = registry.getTransform(camera);
            auto* protagonistT = registry.getTransform(protagonist);
            auto* protagonistFacing = registry.getFacingDirection(protagonist);
            auto* ft = registry.getFollowTarget(camera);
            if (cam && camT && protagonistT && protagonistFacing && ft) {
                float yawRad = glm::radians(protagonistFacing->yaw);
                glm::vec3 forward(-sin(yawRad), 0.0f, -cos(yawRad));
                glm::vec3 lookAtPos = protagonistT->position + forward * ft->lookAhead;
                lookAtPos.y += 1.0f;
                glm::mat4 view = glm::lookAt(camT->position, lookAtPos, glm::vec3(0.0f, 1.0f, 0.0f));
                glm::mat4 vp = cam->projectionMatrix(aspectRatio) * view;
                colorShader.use();
                colorShader.setMat4("uMVP", vp);
                axes.draw();
            }

            renderSystem.update(registry, aspectRatio);
            uiSystem.update(registry, WINDOW_WIDTH, WINDOW_HEIGHT);
        }
        else if (currentScene == SceneType::GodMode) {
            // Return to menu on escape
            if (input.escapePressed) {
                sceneManager.switchTo(SceneType::MainMenu);
            }

            // Free camera control
            freeCameraSystem.update(registry, dt, input.mouseX, input.mouseY);

            // Still update animations for visual effect
            animationSystem.update(registry, dt);
            skeletonSystem.update(registry);

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

                renderSystem.updateWithView(registry, aspectRatio, view);
            }
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
