#pragma once

#include <SDL.h>
#include <glad/glad.h>
#include <glm/glm.hpp>
#include "Camera.h"
#include "Scene.h"
#include "LightManager.h"
#include "Shader.h"

class Renderer
{
public:
    Renderer();
    ~Renderer();

    bool initialize(int width, int height);
    void render(const Camera &camera, Scene &scene, LightManager &lightManager);
    void render(const Camera &camera, Scene &scene, LightManager &lightManager, float timeSeconds, float deltaTime, float overlaySnowSpeed, bool accumEnabled, float accumDecayPerSec, float overlayDirectionDeg, float trailGain, float advectionScale);
    void clearAccumulation();
    void handleResize(int width, int height);
    void cleanup();

    glm::mat4 getProjection() const { return projection; }

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
