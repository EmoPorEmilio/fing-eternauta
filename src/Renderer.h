/**
 * @file Renderer.h
 * @brief SDL2 window and OpenGL 4.5 context management
 *
 * Renderer handles low-level graphics setup and the main render pass.
 * It owns the SDL window, OpenGL context, projection matrix, and optional
 * post-processing resources (overlay/accumulation for snow trails).
 *
 * Initialization:
 *   - Creates SDL2 window (maximized, resizable)
 *   - Requests OpenGL 4.5 core profile
 *   - Loads OpenGL functions via GLAD
 *   - Sets up depth testing, backface culling, sRGB framebuffer
 *   - Creates overlay shaders for optional snow trail effect
 *
 * Render Methods:
 *   - render(camera, scene, lightManager): Basic render pass
 *   - render(..., overlayParams): Render with snow overlay and accumulation
 *
 * Accumulation System:
 *   Double-buffered textures for motion blur/trail effects.
 *   Ping-pong between read/write indices each frame.
 *
 * @see Application, IScene, Camera
 */
#pragma once

#include <SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Camera.h"
#include "IScene.h"
#include "LightManager.h"
#include "Shader.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool initialize(int width, int height);
    void render(const Camera &camera, IScene &scene, LightManager &lightManager);
    void render(const Camera &camera, IScene &scene, LightManager &lightManager, float timeSeconds, float deltaTime, float overlaySnowSpeed, bool accumEnabled, float accumDecayPerSec, float overlayDirectionDeg, float trailGain, float advectionScale);
    void clearAccumulation();
    void handleResize(int width, int height);
    void cleanup();

    glm::mat4 getProjection() const { return projection; }
    int getWidth() const { return windowWidth; }
    int getHeight() const { return windowHeight; }
    void setWindowTitle(const char* title);

public:
    SDL_Window *window;
    SDL_GLContext getGLContext() const { return glContext; }

private:
    SDL_GLContext glContext;
    glm::mat4 projection;
    int windowWidth, windowHeight;

    void setupOpenGL();
    void updateProjection();

    // Overlay resources
    Shader overlayShader;
    GLuint fsTriangleVAO = 0;

    // Accumulation resources
    Shader overlayAccumShader;
    Shader presentShader;
    GLuint accumTex[2] = {0, 0};
    GLuint accumFBO[2] = {0, 0};
    int accumReadIdx = 0;
    int accumWriteIdx = 1;
    int accumWidth = 0;
    int accumHeight = 0;

    void destroyAccumResources();
    bool createAccumResources(int width, int height);
};
