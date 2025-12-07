// fing-eternauta - GLB Model Loader with Skeletal Animation
// Controls: WASD move, Right-click drag look, Space/Shift up/down, ESC exit

#include <glad/glad.h>
#include <SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

#include "src/Shader.h"
#include "src/Model.h"
#include "src/Camera.h"
#include "src/DebugRenderer.h"

int main(int argc, char* argv[])
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
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

    if (!window)
    {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        SDL_Quit();
        return -1;
    }

    SDL_GLContext glContext = SDL_GL_CreateContext(window);
    if (!glContext)
    {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        SDL_GL_DeleteContext(glContext);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return -1;
    }

    std::cout << "OpenGL " << glGetString(GL_VERSION) << std::endl;

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    glEnable(GL_DEPTH_TEST);

    // Load shaders
    Shader colorShader, modelShader, skinnedShader;
    if (!colorShader.loadFromFiles("shaders/color.vert", "shaders/color.frag") ||
        !modelShader.loadFromFiles("shaders/model.vert", "shaders/model.frag") ||
        !skinnedShader.loadFromFiles("shaders/skinned.vert", "shaders/model.frag"))
    {
        std::cerr << "Failed to load shaders" << std::endl;
        return -1;
    }

    // Load models
    Model snowTile, playerCharacter;
    if (!snowTile.loadFromFile("assets/snow_tile.glb") ||
        !playerCharacter.loadFromFile("assets/player_character/Animation_Walking_withSkin.glb"))
    {
        std::cerr << "Failed to load models" << std::endl;
        return -1;
    }

    AxisRenderer axes;
    axes.init();

    Camera camera;
    camera.updateVectors();

    glm::mat4 projection = glm::perspective(
        glm::radians(60.0f),
        (float)WINDOW_WIDTH / (float)WINDOW_HEIGHT,
        0.1f, 100.0f
    );

    glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));

    Uint64 prevTime = SDL_GetPerformanceCounter();
    Uint64 frequency = SDL_GetPerformanceFrequency();

    bool running = true;
    bool mouseHeld = false;
    SDL_Event event;

    while (running)
    {
        Uint64 currentTime = SDL_GetPerformanceCounter();
        float deltaTime = (float)(currentTime - prevTime) / frequency;
        prevTime = currentTime;

        while (SDL_PollEvent(&event))
        {
            if (event.type == SDL_QUIT)
                running = false;
            else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE)
                running = false;
            else if (event.type == SDL_MOUSEBUTTONDOWN && event.button.button == SDL_BUTTON_RIGHT)
            {
                mouseHeld = true;
                SDL_SetRelativeMouseMode(SDL_TRUE);
            }
            else if (event.type == SDL_MOUSEBUTTONUP && event.button.button == SDL_BUTTON_RIGHT)
            {
                mouseHeld = false;
                SDL_SetRelativeMouseMode(SDL_FALSE);
            }
            else if (event.type == SDL_MOUSEMOTION && mouseHeld)
            {
                camera.processMouseMovement((float)event.motion.xrel, (float)event.motion.yrel);
            }
        }

        camera.processKeyboard(SDL_GetKeyboardState(nullptr), deltaTime);
        playerCharacter.updateAnimation(deltaTime);

        glm::mat4 view = camera.getViewMatrix();

        glClearColor(0.1f, 0.1f, 0.12f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw axes
        colorShader.use();
        colorShader.setMat4("uMVP", projection * view);
        axes.draw();

        // Draw snow tiles
        modelShader.use();
        modelShader.setMat4("uView", view);
        modelShader.setMat4("uProjection", projection);
        modelShader.setVec3("uLightDir", lightDir);
        modelShader.setVec3("uViewPos", camera.position);
        modelShader.setInt("uTexture", 0);
        modelShader.setInt("uHasTexture", snowTile.hasTextures() ? 1 : 0);

        const float tileSize = 1.7f;
        for (int x = -2; x <= 2; x++)
        {
            for (int z = -2; z <= 2; z++)
            {
                glm::mat4 snowModel = glm::translate(glm::mat4(1.0f),
                    glm::vec3(x * tileSize, 0.0f, z * tileSize));
                modelShader.setMat4("uModel", snowModel);
                snowTile.draw();
            }
        }

        // Draw player character
        skinnedShader.use();
        skinnedShader.setMat4("uView", view);
        skinnedShader.setMat4("uProjection", projection);
        skinnedShader.setVec3("uLightDir", lightDir);
        skinnedShader.setVec3("uViewPos", camera.position);
        skinnedShader.setInt("uTexture", 0);

        glm::mat4 playerModelMat = glm::scale(glm::mat4(1.0f), glm::vec3(0.01f));
        skinnedShader.setMat4("uModel", playerModelMat);
        skinnedShader.setInt("uHasTexture", playerCharacter.hasTextures() ? 1 : 0);
        skinnedShader.setInt("uUseSkinning", playerCharacter.hasAnimations() ? 1 : 0);
        skinnedShader.setMat4Array("uBones", playerCharacter.getBoneMatrices());
        playerCharacter.draw();

        SDL_GL_SwapWindow(window);
    }

    axes.cleanup();
    SDL_GL_DeleteContext(glContext);
    SDL_DestroyWindow(window);
    SDL_Quit();

    return 0;
}
