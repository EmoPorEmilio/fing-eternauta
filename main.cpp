// fing-eternauta - ECS-based GLB Model Loader with Skeletal Animation
// Controls: WASD move character, Mouse rotate view, ESC exit

#include <glad/glad.h>
#include <SDL.h>
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
#include "src/assets/AssetLoader.h"
#include "src/DebugRenderer.h"
#include "src/Shader.h"

int main(int argc, char* argv[]) {
    // SDL init
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
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
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN
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
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    std::cout << "OpenGL " << glGetString(GL_VERSION) << std::endl;

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);

    // Registry
    Registry registry;

    // Systems
    InputSystem inputSystem;
    PlayerMovementSystem playerMovementSystem;
    CameraOrbitSystem cameraOrbitSystem;
    FollowCameraSystem followCameraSystem;
    AnimationSystem animationSystem;
    SkeletonSystem skeletonSystem;
    PhysicsSystem physicsSystem;
    CollisionSystem collisionSystem;
    RenderSystem renderSystem;
    renderSystem.loadShaders();

    inputSystem.captureMouse(true);

    // Load assets
    LoadedModel protagonistData = loadGLB("assets/protagonist.glb");
    LoadedModel snowData = loadGLB("assets/snow_tile.glb");

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

    if (protagonistData.skeleton) {
        registry.addSkeleton(protagonist, std::move(*protagonistData.skeleton));
        animationSystem.setClips(protagonist, std::move(protagonistData.clips));

        // Add animation component (starts not playing, will play when moving)
        Animation anim;
        anim.clipIndex = 0;
        anim.playing = false;
        registry.addAnimation(protagonist, anim);
    }

    // Create snow tiles with box colliders
    const float tileSize = 1.7f;
    const float tileHeight = 0.25f;  // Approximate height of snow tile mesh
    for (int x = -2; x <= 2; x++) {
        for (int z = -2; z <= 2; z++) {
            Entity tile = registry.create();
            Transform tileTransform;
            tileTransform.position = glm::vec3(x * tileSize, 0.0f, z * tileSize);
            registry.addTransform(tile, tileTransform);
            registry.addMeshGroup(tile, snowData.meshGroup);
            Renderable tileRenderable;
            tileRenderable.shader = ShaderType::Model;
            registry.addRenderable(tile, tileRenderable);

            // Add box collider for ground collision
            BoxCollider tileCollider;
            tileCollider.halfExtents = glm::vec3(tileSize * 0.5f, tileHeight * 0.5f, tileSize * 0.5f);
            tileCollider.offset = glm::vec3(0.0f, tileHeight * 0.5f, 0.0f);
            registry.addBoxCollider(tile, tileCollider);
        }
    }

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

    // Debug axes
    Shader colorShader;
    colorShader.loadFromFiles("shaders/color.vert", "shaders/color.frag");
    AxisRenderer axes;
    axes.init();

    // Timing
    Uint64 prevTime = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();
    float aspectRatio = (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT;

    // Game loop
    bool running = true;
    while (running) {
        Uint64 currentTime = SDL_GetPerformanceCounter();
        float dt = (float)(currentTime - prevTime) / frequency;
        prevTime = currentTime;

        // Poll input events
        InputState input = inputSystem.pollEvents();
        running = !input.quit;

        // Get camera yaw for player movement direction
        auto* ft = registry.getFollowTarget(camera);
        float cameraYaw = ft ? ft->yaw : 0.0f;

        // Update systems in order
        cameraOrbitSystem.update(registry, input.mouseX, input.mouseY);
        playerMovementSystem.update(registry, dt, cameraYaw);
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
        if (cam && camT && protagonistT && ft) {
            // Look ahead in facing direction
            float yawRad = glm::radians(ft->yaw);
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

        SDL_GL_SwapWindow(window);
    }

    // Cleanup
    axes.cleanup();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
