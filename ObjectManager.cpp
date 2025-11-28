#include "ObjectManager.h"
#include <glad/glad.h>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <iomanip>

std::string loadShaderFile(const std::string &filename)
{
    std::ifstream file(filename);
    if (!file.is_open())
    {
        std::cerr << "Failed to open shader file: " << filename << std::endl;
        return "";
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

ObjectManager::ObjectManager() : isInitialized(false),
                                 highLodVao(0), highLodVbo(0), highLodEbo(0),
                                 mediumLodVao(0), mediumLodVbo(0), mediumLodEbo(0),
                                 lowLodVao(0), lowLodVbo(0), lowLodEbo(0),
                                 highLodInstanceVbo(0), mediumLodInstanceVbo(0), lowLodInstanceVbo(0),
                                 shaderProgram(0), frameTime(0.0f), fps(0.0f),
                                 frameCount(0), lastStatsTime(0.0f),
                                 cullingEnabled(true), lodEnabled(false),
                                 fogEnabled(true), fogColor(0.0f, 0.0f, 0.0f), fogDensity(0.01f), fogDesaturationStrength(1.0f), fogAbsorptionDensity(0.02f), fogAbsorptionStrength(0.8f)
{
}

ObjectManager::~ObjectManager()
{
    cleanup();
}

void ObjectManager::initialize(int objectCount)
{
    if (isInitialized)
    {
        cleanup();
    }

    std::cout << "Loading " << objectCount << " objects..." << std::endl;

    // Setup OpenGL resources first
    setupLODGeometry();
    setupShader();

    // Force shader recompilation to ensure latest changes are applied
    if (shaderProgram != 0)
    {
        std::cout << "ObjectManager: Forcing shader recompilation..." << std::endl;
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }
    setupShader();
    std::cout << "ObjectManager: Shader recompiled successfully!" << std::endl;

    // Generate all objects at once with progress bar
    generateGridPositions(objectCount);

    isInitialized = true;

    std::cout << "Loading complete! Generated " << objects.size() << " objects." << std::endl;
}

void ObjectManager::update(const glm::vec3 &cameraPos, float cullDistance, float highLodDistance, float mediumLodDistance, float deltaTime)
{
    if (!isInitialized)
        return;

    // Update performance stats
    if (deltaTime > 0.0f)
        updatePerformanceStats(deltaTime);

    updateObjectDistances(cameraPos);

    // Apply culling based on configuration
    if (cullingEnabled)
    {
        cullObjects(cullDistance);
    }
    else
    {
        // Since culling is disabled, mark all objects as visible
        static bool cullingDisabledMessageShown = false;
        if (!cullingDisabledMessageShown)
        {
            std::cout << "CULLING DISABLED: All " << objects.size() << " objects will be rendered!" << std::endl;
            cullingDisabledMessageShown = true;
        }
        for (auto &obj : objects)
        {
            obj.isVisible = true;
        }
    }

    // Show LOD status message
    static bool lodDisabledMessageShown = false;
    if (!lodEnabled && !lodDisabledMessageShown)
    {
        std::cout << "LOD DISABLED: All objects will use high detail geometry!" << std::endl;
        lodDisabledMessageShown = true;
    }

    // Only recalculate LOD every 10 frames to reduce CPU overhead
    static int lodUpdateCounter = 0;
    bool shouldUpdateLOD = (++lodUpdateCounter >= 10);
    if (shouldUpdateLOD)
    {
        lodUpdateCounter = 0;
    }

    // Update LOD levels and model matrices for visible objects
    for (auto &obj : objects)
    {
        if (obj.isVisible)
        {
            // Determine LOD level based on distance and configuration
            if (lodEnabled && shouldUpdateLOD)
            {
                // Normal LOD calculation (only every 10 frames)
                if (obj.distanceToCamera <= highLodDistance)
                {
                    obj.currentLOD = LODLevel::HIGH;
                }
                else if (obj.distanceToCamera <= mediumLodDistance)
                {
                    obj.currentLOD = LODLevel::MEDIUM;
                }
                else
                {
                    obj.currentLOD = LODLevel::LOW;
                }
            }
            else if (!lodEnabled)
            {
                // LOD disabled - all objects use high detail
                obj.currentLOD = LODLevel::HIGH;
            }
            // If LOD enabled but not updating this frame, keep current LOD level

            obj.modelMatrix = glm::translate(glm::mat4(1.0f), obj.position);
        }
    }
}

void ObjectManager::render(const glm::mat4 &view, const glm::mat4 &projection, const glm::vec3 &cameraPos, const glm::vec3 &cameraFront, const LightManager &lightManager, GLuint textureID)
{
    if (!isInitialized)
        return;

    glUseProgram(shaderProgram);

    // Set common uniforms
    GLint viewLoc = glGetUniformLocation(shaderProgram, "uView");
    GLint projLoc = glGetUniformLocation(shaderProgram, "uProj");
    GLint viewPosLoc = glGetUniformLocation(shaderProgram, "uViewPos");
    GLint objectColorLoc = glGetUniformLocation(shaderProgram, "uObjectColor");
    GLint useTextureLoc = glGetUniformLocation(shaderProgram, "uUseTexture");
    GLint flashlightOnLoc = glGetUniformLocation(shaderProgram, "uFlashlightOn");
    GLint flashlightPosLoc = glGetUniformLocation(shaderProgram, "uFlashlightPos");
    GLint flashlightDirLoc = glGetUniformLocation(shaderProgram, "uFlashlightDir");
    GLint flashlightCutoffLoc = glGetUniformLocation(shaderProgram, "uFlashlightCutoff");
    GLint flashlightBrightnessLoc = glGetUniformLocation(shaderProgram, "uFlashlightBrightness");
    GLint flashlightColorLoc = glGetUniformLocation(shaderProgram, "uFlashlightColor");
    GLint fogEnabledLoc = glGetUniformLocation(shaderProgram, "uFogEnabled");
    GLint fogColorLoc = glGetUniformLocation(shaderProgram, "uFogColor");
    GLint fogDensityLoc = glGetUniformLocation(shaderProgram, "uFogDensity");
    GLint fogDesaturationStrengthLoc = glGetUniformLocation(shaderProgram, "uFogDesaturationStrength");
    GLint fogAbsorptionDensityLoc = glGetUniformLocation(shaderProgram, "uFogAbsorptionDensity");
    GLint fogAbsorptionStrengthLoc = glGetUniformLocation(shaderProgram, "uFogAbsorptionStrength");
    GLint backgroundColorLoc = glGetUniformLocation(shaderProgram, "uBackgroundColor");

    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, glm::value_ptr(projection));
    glUniform3f(viewPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform3f(objectColorLoc, 1.0f, 0.0f, 0.0f); // Red color
    glUniform1i(useTextureLoc, 0);                 // No texture for objects

    // Set flashlight uniforms
    glUniform1i(flashlightOnLoc, lightManager.isFlashlightOn() ? 1 : 0);
    glUniform3f(flashlightPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
    glUniform3f(flashlightDirLoc, cameraFront.x, cameraFront.y, cameraFront.z);
    glUniform1f(flashlightCutoffLoc, lightManager.getFlashlightCutoff());
    glUniform1f(flashlightBrightnessLoc, lightManager.getFlashlightBrightness());
    glm::vec3 flColor = lightManager.getFlashlightColor();
    glUniform3f(flashlightColorLoc, flColor.x, flColor.y, flColor.z);

    // Set fog uniforms
    glUniform1i(fogEnabledLoc, fogEnabled ? 1 : 0);
    glUniform3f(fogColorLoc, fogColor.x, fogColor.y, fogColor.z);
    glUniform1f(fogDensityLoc, fogDensity);
    glUniform1f(fogDesaturationStrengthLoc, fogDesaturationStrength);
    glUniform1f(fogAbsorptionDensityLoc, fogAbsorptionDensity);
    glUniform1f(fogAbsorptionStrengthLoc, fogAbsorptionStrength);
    glUniform3f(backgroundColorLoc, 0.08f, 0.1f, 0.12f); // Match renderer clear color

    // Debug: Check if uniform location is valid and log values
    if (fogDesaturationStrengthLoc == -1)
    {
        std::cout << "ERROR: uFogDesaturationStrength uniform not found in ObjectManager shader!" << std::endl;
    }
    else
    {
        static int debugCounter = 0;
        if (debugCounter % 60 == 0)
        { // Log every 60 frames
            std::cout << "ObjectManager Debug - FogEnabled: " << fogEnabled
                      << ", FogDensity: " << fogDensity
                      << ", FogDesaturation: " << fogDesaturationStrength << std::endl;
        }
        debugCounter++;
    }

    // Gather instances per LOD - reserve smart estimates to prevent reallocations
    std::vector<glm::mat4> highInstances;
    std::vector<glm::mat4> mediumInstances;
    std::vector<glm::mat4> lowInstances;

    // Smart memory reservation based on typical LOD distribution
    // Estimate: 10% high, 25% medium, 65% low LOD based on new distances
    size_t totalObjects = objects.size();
    highInstances.reserve(totalObjects / 10);   // ~10% expected in high LOD
    mediumInstances.reserve(totalObjects / 4);  // ~25% expected in medium LOD
    lowInstances.reserve(totalObjects * 2 / 3); // ~65% expected in low LOD

    // Fast path: if LOD is disabled, use single batch rendering
    if (!lodEnabled)
    {
        for (const auto &obj : objects)
        {
            if (obj.isVisible)
                highInstances.push_back(obj.modelMatrix);
        }

        if (!highInstances.empty())
            renderLODLevelInstanced(LODLevel::HIGH, highInstances);
    }
    else
    {
        // LOD enabled: sort into buckets
        for (const auto &obj : objects)
        {
            if (!obj.isVisible)
                continue;
            if (obj.currentLOD == LODLevel::HIGH)
                highInstances.push_back(obj.modelMatrix);
            else if (obj.currentLOD == LODLevel::MEDIUM)
                mediumInstances.push_back(obj.modelMatrix);
            else
                lowInstances.push_back(obj.modelMatrix);
        }

        // Render each LOD level with instancing
        if (!highInstances.empty())
            renderLODLevelInstanced(LODLevel::HIGH, highInstances);
        if (!mediumInstances.empty())
            renderLODLevelInstanced(LODLevel::MEDIUM, mediumInstances);
        if (!lowInstances.empty())
            renderLODLevelInstanced(LODLevel::LOW, lowInstances);

        // Debug info: show LOD distribution occasionally
        static int lodDebugCounter = 0;
        if (++lodDebugCounter >= 300)
        { // Every 5 seconds at 60fps
            lodDebugCounter = 0;
            std::cout << "LOD Distribution - High: " << highInstances.size()
                      << ", Medium: " << mediumInstances.size()
                      << ", Low: " << lowInstances.size()
                      << " (Total visible: " << (highInstances.size() + mediumInstances.size() + lowInstances.size()) << ")" << std::endl;
        }
    }

    // Print performance info every 60 frames (gated by OPENGL_ADV_DEBUG_LOGS)
    static bool gDebugLogs = (std::getenv("OPENGL_ADV_DEBUG_LOGS") != nullptr);
    if (gDebugLogs)
    {
        static int frameCounter = 0;
        frameCounter++;
        if (frameCounter % 60 == 0)
        {
            std::cout << "Rendered " << (highInstances.size() + mediumInstances.size() + lowInstances.size())
                      << " objects (High: " << highInstances.size()
                      << ", Medium: " << mediumInstances.size()
                      << ", Low: " << lowInstances.size() << ")" << std::endl;
        }
    }
}

void ObjectManager::setObjectCount(int count)
{
    if (count != static_cast<int>(objects.size()))
    {
        initialize(count);
    }
}

void ObjectManager::generateGridPositions(int objectCount)
{
    objects.clear();
    objects.reserve(objectCount);

    // Calculate grid dimensions
    float gridSpacing = MIN_DISTANCE;
    int gridSize = static_cast<int>(std::ceil(std::sqrt(objectCount)));

    // Center the grid around origin
    float startX = -(gridSize * gridSpacing) / 2.0f;
    float startZ = -(gridSize * gridSpacing) / 2.0f;

    int objectsGenerated = 0;
    const int progressInterval = std::max(1, objectCount / 20); // Show progress every 5%

    std::cout << "Generating grid of " << objectCount << " objects..." << std::endl;

    for (int row = 0; row < gridSize && objectsGenerated < objectCount; row++)
    {
        for (int col = 0; col < gridSize && objectsGenerated < objectCount; col++)
        {
            GameObject obj;
            obj.position = glm::vec3(
                startX + col * gridSpacing,
                0.5f, // Y position (half height of prism)
                startZ + row * gridSpacing);

            // Initialize object properties
            obj.modelMatrix = glm::translate(glm::mat4(1.0f), obj.position);
            obj.distanceToCamera = 0.0f;
            obj.isVisible = true;
            obj.currentLOD = LODLevel::HIGH;

            objects.push_back(obj);
            objectsGenerated++;

            // Show progress
            if (objectsGenerated % progressInterval == 0 || objectsGenerated == objectCount)
            {
                float progress = (float)objectsGenerated / objectCount * 100.0f;
                int barWidth = 50;
                int filled = static_cast<int>((progress / 100.0f) * barWidth);

                std::cout << "\r[";
                for (int i = 0; i < barWidth; i++)
                {
                    if (i < filled)
                        std::cout << "=";
                    else if (i == filled)
                        std::cout << ">";
                    else
                        std::cout << " ";
                }
                std::cout << "] " << std::fixed << std::setprecision(1) << progress << "% ("
                          << objectsGenerated << "/" << objectCount << ")" << std::flush;
            }
        }
    }

    std::cout << std::endl; // New line after progress bar
}

void ObjectManager::setupGeometry()
{
    // This method is now replaced by setupLODGeometry()
    // Keeping it for compatibility but it does nothing
}

void ObjectManager::setupShader()
{
    // Use AssetManager to load shader program with proper error checking
    shaderProgram = AssetManager::loadShaderProgram(
        "object_instanced.vert",
        "object_instanced.frag",
        "ObjectManager_instanced");

    if (shaderProgram == 0)
    {
        std::cerr << "CRITICAL ERROR: Failed to load ObjectManager shader program!" << std::endl;
        std::cerr << "This will cause rendering failures. Check shader files and OpenGL context." << std::endl;
    }
    else
    {
        std::cout << "ObjectManager: Shader program loaded successfully (ID: " << shaderProgram << ")" << std::endl;
    }

    // Verify that the OpenGL context is still valid after shader operations
    AssetManager::checkGLError("ObjectManager::setupShader");
}

void ObjectManager::updateObjectDistances(const glm::vec3 &cameraPos)
{
    for (auto &obj : objects)
    {
        obj.distanceToCamera = glm::length(obj.position - cameraPos);
    }
}

void ObjectManager::cullObjects(float cullDistance)
{
    for (auto &obj : objects)
    {
        obj.isVisible = obj.distanceToCamera <= cullDistance;
    }
}

bool ObjectManager::isValidPosition(const glm::vec3 &pos)
{
    // Check world bounds
    if (std::abs(pos.x) > WORLD_SIZE || std::abs(pos.z) > WORLD_SIZE)
    {
        return false;
    }

    // Check minimum distance from other objects
    for (const auto &other : objects)
    {
        if (glm::length(pos - other.position) < MIN_DISTANCE)
        {
            return false;
        }
    }

    return true;
}

void ObjectManager::setupLODGeometry()
{
    // Generate geometry for each LOD level
    // Use randomized high LOD to prevent GPU batching optimization
    highLodPrism.generateRandomizedHighLODGeometry(12345); // Use a fixed seed for consistency
    mediumLodPrism.generateGeometry(LODLevel::MEDIUM);
    lowLodPrism.generateGeometry(LODLevel::LOW);

    // Setup VAOs for each LOD level (with instance buffers)
    setupLODVAO(highLodVao, highLodVbo, highLodEbo, highLodInstanceVbo, highLodPrism);
    setupLODVAO(mediumLodVao, mediumLodVbo, mediumLodEbo, mediumLodInstanceVbo, mediumLodPrism);
    setupLODVAO(lowLodVao, lowLodVbo, lowLodEbo, lowLodInstanceVbo, lowLodPrism);

    std::cout << "LOD Geometry: High=" << highLodPrism.getTriangleCount()
              << " triangles, Medium=" << mediumLodPrism.getTriangleCount()
              << " triangles, Low=" << lowLodPrism.getTriangleCount() << " triangles" << std::endl;
}

void ObjectManager::setupLODVAO(GLuint &vao, GLuint &vbo, GLuint &ebo, GLuint &instanceVbo, const Prism &prism)
{
    const auto &vertices = prism.getVertices();
    const auto &indices = prism.getIndices();

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);
    glGenBuffers(1, &ebo);

    glBindVertexArray(vao);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, static_cast<GLsizeiptr>(vertices.size() * sizeof(Prism::Vertex)), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, static_cast<GLsizeiptr>(indices.size() * sizeof(unsigned int)), indices.data(), GL_STATIC_DRAW);

    // Position attribute
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Prism::Vertex), (void *)0);

    // Normal attribute
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Prism::Vertex), (void *)offsetof(Prism::Vertex, normal));

    // UV attribute
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Prism::Vertex), (void *)offsetof(Prism::Vertex, uv));

    // Instance buffer for model matrices (mat4 -> 4 vec4 attributes)
    glGenBuffers(1, &instanceVbo);
    glBindBuffer(GL_ARRAY_BUFFER, instanceVbo);
    glBufferData(GL_ARRAY_BUFFER, 0, nullptr, GL_DYNAMIC_DRAW);

    std::size_t vec4Size = sizeof(glm::vec4);
    GLsizei stride = sizeof(glm::mat4);
    // Attribute locations 3,4,5,6 for iModel
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, stride, (void *)0);
    glVertexAttribDivisor(3, 1);
    glEnableVertexAttribArray(4);
    glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, stride, (void *)(1 * vec4Size));
    glVertexAttribDivisor(4, 1);
    glEnableVertexAttribArray(5);
    glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, stride, (void *)(2 * vec4Size));
    glVertexAttribDivisor(5, 1);
    glEnableVertexAttribArray(6);
    glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, stride, (void *)(3 * vec4Size));
    glVertexAttribDivisor(6, 1);

    glBindVertexArray(0);
}

void ObjectManager::renderLODLevelInstanced(LODLevel lod, const std::vector<glm::mat4> &instanceMatrices)
{
    GLuint vao = 0;
    GLuint *instanceVboPtr = nullptr;
    const Prism *prism = nullptr;

    switch (lod)
    {
    case LODLevel::HIGH:
        vao = highLodVao;
        prism = &highLodPrism;
        instanceVboPtr = &highLodInstanceVbo;
        break;
    case LODLevel::MEDIUM:
        vao = mediumLodVao;
        prism = &mediumLodPrism;
        instanceVboPtr = &mediumLodInstanceVbo;
        break;
    case LODLevel::LOW:
        vao = lowLodVao;
        prism = &lowLodPrism;
        instanceVboPtr = &lowLodInstanceVbo;
        break;
    }

    // Safety check
    if (!prism || vao == 0 || instanceVboPtr == nullptr)
    {
        return;
    }

    glBindVertexArray(vao);

    // Update instance buffer with matrices
    glBindBuffer(GL_ARRAY_BUFFER, *instanceVboPtr);
    glBufferData(GL_ARRAY_BUFFER, instanceMatrices.size() * sizeof(glm::mat4), instanceMatrices.data(), GL_DYNAMIC_DRAW);

    // Draw instanced
    glDrawElementsInstanced(GL_TRIANGLES, static_cast<GLsizei>(prism->getIndices().size()), GL_UNSIGNED_INT, 0, static_cast<GLsizei>(instanceMatrices.size()));

    glBindVertexArray(0);
}

void ObjectManager::cleanup()
{
    if (highLodVao != 0)
    {
        glDeleteVertexArrays(1, &highLodVao);
        glDeleteBuffers(1, &highLodVbo);
        glDeleteBuffers(1, &highLodEbo);
        highLodVao = highLodVbo = highLodEbo = 0;
    }
    if (mediumLodVao != 0)
    {
        glDeleteVertexArrays(1, &mediumLodVao);
        glDeleteBuffers(1, &mediumLodVbo);
        glDeleteBuffers(1, &mediumLodEbo);
        mediumLodVao = mediumLodVbo = mediumLodEbo = 0;
    }
    if (lowLodVao != 0)
    {
        glDeleteVertexArrays(1, &lowLodVao);
        glDeleteBuffers(1, &lowLodVbo);
        glDeleteBuffers(1, &lowLodEbo);
        lowLodVao = lowLodVbo = lowLodEbo = 0;
    }
    if (shaderProgram != 0)
    {
        glDeleteProgram(shaderProgram);
        shaderProgram = 0;
    }

    isInitialized = false;
}

void ObjectManager::updatePerformanceStats(float deltaTime)
{
    frameTime = deltaTime;
    fps = 1.0f / deltaTime;
    frameCount++;

    static float totalTime = 0.0f;
    totalTime += deltaTime;

    if (totalTime >= STATS_INTERVAL)
    {
        printPerformanceStats();
        totalTime = 0.0f;
    }
}

void ObjectManager::printPerformanceStats()
{
    // Count objects by LOD level
    int highLodCount = 0, mediumLodCount = 0, lowLodCount = 0;
    for (const auto &obj : objects)
    {
        if (!obj.isVisible)
            continue;

        switch (obj.currentLOD)
        {
        case LODLevel::HIGH:
            highLodCount++;
            break;
        case LODLevel::MEDIUM:
            mediumLodCount++;
            break;
        case LODLevel::LOW:
            lowLodCount++;
            break;
        }
    }

    int totalVisible = highLodCount + mediumLodCount + lowLodCount;

    // Calculate geometry complexity
    size_t highVertices = highLodCount * highLodPrism.getVertexCount();
    size_t mediumVertices = mediumLodCount * mediumLodPrism.getVertexCount();
    size_t lowVertices = lowLodCount * lowLodPrism.getVertexCount();
    size_t totalVertices = highVertices + mediumVertices + lowVertices;

    size_t highTriangles = highLodCount * highLodPrism.getTriangleCount();
    size_t mediumTriangles = mediumLodCount * mediumLodPrism.getTriangleCount();
    size_t lowTriangles = lowLodCount * lowLodPrism.getTriangleCount();
    size_t totalTriangles = highTriangles + mediumTriangles + lowTriangles;

    std::cout << "\n=== PERFORMANCE STATS ===" << std::endl;
    std::cout << "FPS: " << std::fixed << std::setprecision(1) << fps << std::endl;
    std::cout << "Frame Time: " << std::fixed << std::setprecision(3) << (frameTime * 1000.0f) << "ms" << std::endl;
    std::cout << "Total Objects: " << objects.size() << std::endl;
    std::cout << "Visible Objects: " << totalVisible << std::endl;
    std::cout << "LOD Distribution:" << std::endl;
    std::cout << "  High LOD: " << highLodCount << " (" << std::fixed << std::setprecision(1) << (highLodCount * 100.0f / totalVisible) << "%)" << std::endl;
    std::cout << "  Medium LOD: " << mediumLodCount << " (" << std::fixed << std::setprecision(1) << (mediumLodCount * 100.0f / totalVisible) << "%)" << std::endl;
    std::cout << "  Low LOD: " << lowLodCount << " (" << std::fixed << std::setprecision(1) << (lowLodCount * 100.0f / totalVisible) << "%)" << std::endl;
    std::cout << "Geometry Complexity:" << std::endl;
    std::cout << "  Total Vertices: " << totalVertices << " (H:" << highVertices << " M:" << mediumVertices << " L:" << lowVertices << ")" << std::endl;
    std::cout << "  Total Triangles: " << totalTriangles << " (H:" << highTriangles << " M:" << mediumTriangles << " L:" << lowTriangles << ")" << std::endl;
    std::cout << "Culled: " << (objects.size() - totalVisible) << std::endl;
    std::cout << "========================" << std::endl;
}
