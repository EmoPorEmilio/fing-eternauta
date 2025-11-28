#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <random>
#include "Prism.h"
#include "LightManager.h"
#include "AssetManager.h"

struct GameObject
{
    glm::vec3 position;
    glm::mat4 modelMatrix;
    float distanceToCamera;
    bool isVisible;
    LODLevel currentLOD;

    GameObject() : position(0.0f), modelMatrix(1.0f), distanceToCamera(0.0f), isVisible(true), currentLOD(LODLevel::HIGH) {}
};

class ObjectManager
{
public:
    ObjectManager();
    ~ObjectManager();

    void initialize(int objectCount = PRESET_MAXIMUM);
    void update(const glm::vec3 &cameraPos, float cullDistance, float highLodDistance, float mediumLodDistance, float deltaTime = 0.0f);
    void render(const glm::mat4 &view, const glm::mat4 &projection, const glm::vec3 &cameraPos, const glm::vec3 &cameraFront, const LightManager &lightManager, GLuint textureID);

    void setObjectCount(int count);
    int getObjectCount() const { return static_cast<int>(objects.size()); }

    // Performance presets
    static constexpr int PRESET_MINIMAL = 3000;   // Current amount (minimal)
    static constexpr int PRESET_MEDIUM = 15000;   // Much more objects
    static constexpr int PRESET_MAXIMUM = 500000; // A MASSIVE SHITTON of objects (10x increase)

    // Performance monitoring
    void printPerformanceStats();
    void updatePerformanceStats(float deltaTime);

    // Runtime configuration
    void setCullingEnabled(bool enabled) { cullingEnabled = enabled; }
    void setLODEnabled(bool enabled) { lodEnabled = enabled; }
    bool isCullingEnabled() const { return cullingEnabled; }
    bool isLODEnabled() const { return lodEnabled; }
    void toggleCulling() { cullingEnabled = !cullingEnabled; }
    void toggleLOD() { lodEnabled = !lodEnabled; }

    // Fog configuration
    void setFogEnabled(bool enabled) { fogEnabled = enabled; }
    void setFogColor(const glm::vec3 &color) { fogColor = color; }
    void setFogDensity(float density) { fogDensity = density; }
    void setFogDesaturationStrength(float strength) { fogDesaturationStrength = strength; }
    void setFogAbsorption(float density, float strength)
    {
        fogAbsorptionDensity = density;
        fogAbsorptionStrength = strength;
    }

    void cleanup();

private:
    std::vector<GameObject> objects;
    Prism prismGeometry;

    // OpenGL objects - multiple VAOs for different LOD levels
    GLuint highLodVao, highLodVbo, highLodEbo;
    GLuint mediumLodVao, mediumLodVbo, mediumLodEbo;
    GLuint lowLodVao, lowLodVbo, lowLodEbo;
    // Instance buffers for model matrices per LOD
    GLuint highLodInstanceVbo;
    GLuint mediumLodInstanceVbo;
    GLuint lowLodInstanceVbo;
    GLuint shaderProgram;

    // LOD geometry data
    Prism highLodPrism, mediumLodPrism, lowLodPrism;

    // Rendering state
    bool isInitialized;

    // Performance monitoring
    float frameTime;
    float fps;
    int frameCount;
    float lastStatsTime;
    static constexpr float STATS_INTERVAL = 1.0f; // Print stats every 1 second

    // Runtime configuration
    bool cullingEnabled;
    bool lodEnabled;

    // Fog parameters
    bool fogEnabled;
    glm::vec3 fogColor;
    float fogDensity;
    float fogDesaturationStrength;
    float fogAbsorptionDensity;
    float fogAbsorptionStrength;

    // Constants
    static constexpr float MIN_DISTANCE = 3.0f;          // Minimum distance between objects
    static constexpr float WORLD_SIZE = 500.0f;          // World bounds
    static constexpr float HIGH_LOD_DISTANCE = 50.0f;    // Distance for high LOD (increased for better balance)
    static constexpr float MEDIUM_LOD_DISTANCE = 150.0f; // Distance for medium LOD (increased for better balance)

    void generateGridPositions(int objectCount);
    void setupGeometry();
    void setupShader();
    void updateObjectDistances(const glm::vec3 &cameraPos);
    void cullObjects(float cullDistance);
    bool isValidPosition(const glm::vec3 &pos);

    // LOD-specific setup
    void setupLODGeometry();
    void setupLODVAO(GLuint &vao, GLuint &vbo, GLuint &ebo, GLuint &instanceVbo, const Prism &prism);
    void renderLODLevelInstanced(LODLevel lod, const std::vector<glm::mat4> &instanceMatrices);
};
