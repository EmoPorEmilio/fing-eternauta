#include "Renderer.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Renderer::Renderer() : window(nullptr), glContext(nullptr), windowWidth(960), windowHeight(540)
{
}

Renderer::~Renderer()
{
    cleanup();
}

bool Renderer::initialize(int width, int height)
{
    windowWidth = width;
    windowHeight = height;

    if (SDL_Init(SDL_INIT_VIDEO) != 0)
    {
        std::cerr << "SDL_Init failed: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 4);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 5);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

    // Get display bounds for fullscreen
    SDL_DisplayMode displayMode;
    SDL_GetCurrentDisplayMode(0, &displayMode);
    windowWidth = displayMode.w;
    windowHeight = displayMode.h;

    window = SDL_CreateWindow(
        "Proyecto Viviana - OpenGL Scene Editor",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        windowWidth, windowHeight,
        SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_MAXIMIZED);

    if (!window)
    {
        std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
        return false;
    }

    glContext = SDL_GL_CreateContext(window);
    if (!glContext)
    {
        std::cerr << "SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
        return false;
    }

    SDL_GL_MakeCurrent(window, glContext);
    SDL_GL_SetSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc)SDL_GL_GetProcAddress))
    {
        std::cerr << "Failed to initialize GLAD" << std::endl;
        return false;
    }

    setupOpenGL();
    updateProjection();

    std::cout << "GL Vendor:   " << glGetString(GL_VENDOR) << "\n";
    std::cout << "GL Renderer: " << glGetString(GL_RENDERER) << "\n";
    std::cout << "GL Version:  " << glGetString(GL_VERSION) << "\n";
    std::cout << "GLSL:        " << glGetString(GL_SHADING_LANGUAGE_VERSION) << std::endl;

    // Overlay setup: load shaders and create fullscreen triangle VAO
    overlayShader.loadFromFiles("fullscreen_quad.vert", "shadertoy_overlay.frag");
    overlayAccumShader.loadFromFiles("fullscreen_quad.vert", "overlay_accum.frag");
    presentShader.loadFromFiles("fullscreen_quad.vert", "present.frag");
    glGenVertexArrays(1, &fsTriangleVAO);
    // Create accumulation buffers
    createAccumResources(windowWidth, windowHeight);
    return true;
}

void Renderer::render(const Camera &camera, IScene &scene, LightManager &lightManager)
{
    // Blender-style dark background (#282828)
    glClearColor(0.157f, 0.157f, 0.157f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    scene.render(camera.getViewMatrix(), getProjection(), camera.getPosition(), camera.getFront(), lightManager);
}

void Renderer::render(const Camera &camera, IScene &scene, LightManager &lightManager, float timeSeconds, float deltaTime, float overlaySnowSpeed, bool accumEnabled, float accumDecayPerSec, float overlayDirectionDeg, float trailGain, float advectionScale)
{
    // Blender-style dark background (#282828)
    glClearColor(0.157f, 0.157f, 0.157f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    scene.render(camera.getViewMatrix(), getProjection(), camera.getPosition(), camera.getFront(), lightManager);

    glDisable(GL_DEPTH_TEST);
    if (accumEnabled)
    {
        // Accumulation pass into write FBO
        glBindFramebuffer(GL_FRAMEBUFFER, accumFBO[accumWriteIdx]);
        glViewport(0, 0, accumWidth, accumHeight);
        glDisable(GL_BLEND);
        overlayAccumShader.use();
        overlayAccumShader.setUniform("iResolution", glm::vec3((float)accumWidth, (float)accumHeight, 1.0f));
        overlayAccumShader.setUniform("iTime", timeSeconds);
        overlayAccumShader.setUniform("uDeltaTime", deltaTime);
        overlayAccumShader.setUniform("uSnowSpeed", overlaySnowSpeed);
        overlayAccumShader.setUniform("uAccumDecayPerSec", accumDecayPerSec);
        overlayAccumShader.setUniform("uAccumEnabled", 1);
        overlayAccumShader.setUniform("uSnowDirectionDeg", overlayDirectionDeg);
        overlayAccumShader.setUniform("uTrailGain", trailGain);
        overlayAccumShader.setUniform("uAdvectionScale", advectionScale);
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, accumTex[accumReadIdx]);
        overlayAccumShader.setUniform("uPrevAccum", 0);
        glBindVertexArray(fsTriangleVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // Swap and present
        std::swap(accumReadIdx, accumWriteIdx);
        glViewport(0, 0, windowWidth, windowHeight);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        presentShader.use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, accumTex[accumReadIdx]);
        presentShader.setUniform("uTex", 0);
        glBindVertexArray(fsTriangleVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glDisable(GL_BLEND);
    }
    else
    {
        // Direct overlay (no accumulation)
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        overlayShader.use();
        overlayShader.setUniform("iResolution", glm::vec3((float)windowWidth, (float)windowHeight, 1.0f));
        overlayShader.setUniform("iTime", timeSeconds);
        overlayShader.setUniform("uSnowSpeed", overlaySnowSpeed);
        overlayShader.setUniform("uSnowDirectionDeg", overlayDirectionDeg);
        glBindVertexArray(fsTriangleVAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
        glDisable(GL_BLEND);
    }
    glEnable(GL_DEPTH_TEST);
}

void Renderer::handleResize(int width, int height)
{
    windowWidth = width;
    windowHeight = height;
    glViewport(0, 0, width, height);
    updateProjection();
    // Recreate accumulation buffers to match new size
    destroyAccumResources();
    createAccumResources(width, height);
}

void Renderer::cleanup()
{
    destroyAccumResources();
    if (fsTriangleVAO)
    {
        glDeleteVertexArrays(1, &fsTriangleVAO);
        fsTriangleVAO = 0;
    }
    overlayShader.cleanup();
    overlayAccumShader.cleanup();
    presentShader.cleanup();
    if (glContext)
    {
        SDL_GL_DeleteContext(glContext);
        glContext = nullptr;
    }
    if (window)
    {
        SDL_DestroyWindow(window);
        window = nullptr;
    }
    SDL_Quit();
}

void Renderer::setupOpenGL()
{
    glViewport(0, 0, windowWidth, windowHeight);
    glEnable(GL_DEPTH_TEST);
    // Enable sRGB framebuffer for correct gamma output
    glEnable(GL_FRAMEBUFFER_SRGB);
}

void Renderer::updateProjection()
{
    float aspect = (float)windowWidth / (float)windowHeight;
    projection = glm::perspective(glm::radians(60.0f), aspect, 0.1f, 2000.0f);
}

bool Renderer::createAccumResources(int width, int height)
{
    accumWidth = width;
    accumHeight = height;
    glGenTextures(2, accumTex);
    glGenFramebuffers(2, accumFBO);
    for (int i = 0; i < 2; ++i)
    {
        glBindTexture(GL_TEXTURE_2D, accumTex[i]);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glBindFramebuffer(GL_FRAMEBUFFER, accumFBO[i]);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, accumTex[i], 0);
        GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        if (status != GL_FRAMEBUFFER_COMPLETE)
        {
            std::cerr << "Accum FBO incomplete" << std::endl;
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            return false;
        }
        // Clear to transparent
        glViewport(0, 0, width, height);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    accumReadIdx = 0;
    accumWriteIdx = 1;
    return true;
}

void Renderer::destroyAccumResources()
{
    if (accumFBO[0] || accumFBO[1])
    {
        glDeleteFramebuffers(2, accumFBO);
        accumFBO[0] = accumFBO[1] = 0;
    }
    if (accumTex[0] || accumTex[1])
    {
        glDeleteTextures(2, accumTex);
        accumTex[0] = accumTex[1] = 0;
    }
}

void Renderer::clearAccumulation()
{
    for (int i = 0; i < 2; ++i)
    {
        glBindFramebuffer(GL_FRAMEBUFFER, accumFBO[i]);
        glViewport(0, 0, accumWidth, accumHeight);
        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
    }
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
