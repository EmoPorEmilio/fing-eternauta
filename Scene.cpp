#include "Scene.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

Scene::Scene() : vao(0), vbo(0), cullDistance(200.0f), lodDistance(50.0f)
{
}

Scene::~Scene()
{
    cleanup();
}

bool Scene::initialize()
{
    setupGeometry();
    setupShader();

    // Initialize object manager with maximum objects by default
    objectManager.initialize(); // Uses PRESET_MAXIMUM (50,000 objects)

    // Initialize model manager
    if (!modelManager.initialize())
    {
        std::cerr << "Failed to initialize ModelManager" << std::endl;
        return false;
    }

    // Initialize snow system
    if (!snowSystem.initialize())
    {
        std::cerr << "Failed to initialize SnowSystem" << std::endl;
        return false;
    }

    // Load the FING model - try different paths
    std::vector<std::string> modelPaths = {
        "assets\\modelo_fing.glb",     // In assets subdirectory (Windows path)
        "assets/modelo_fing.glb",      // In assets subdirectory (Unix path)
        "modelo_fing.glb",             // Direct in output directory (fallback)
        "../assets/modelo_fing.glb",   // One level up (fallback)
        "../../assets/modelo_fing.glb" // Two levels up (fallback)
    };

    bool modelLoaded = false;
    for (const auto &path : modelPaths)
    {
        std::cout << "Trying to load model from: " << path << std::endl;
        if (modelManager.loadModel(path, "fing"))
        {
            std::cout << "FING model loaded successfully from: " << path << std::endl;
            modelLoaded = true;
            break;
        }
    }

    if (modelLoaded)
    {
        // Add a MASSIVE instance of the model closer to the camera first
        glm::mat4 modelTransform = glm::mat4(1.0f);
        modelTransform = glm::translate(modelTransform, fingPosition);
        modelTransform = glm::scale(modelTransform, glm::vec3(fingScale));

        fingInstanceId = modelManager.addModelInstance("fing", modelTransform);

        std::cout << "Added FING model instance at (" << fingPosition.x << ", " << fingPosition.y << ", " << fingPosition.z << ") scale " << fingScale << " - instance ID: " << fingInstanceId << std::endl;
    }
    else
    {
        std::cerr << "Failed to load FING model from any path" << std::endl;
    }

    // Load the MILITARY model - try different paths - COMMENTED OUT FOR DEBUGGING
    /*
    std::vector<std::string> militaryPaths = {
        "assets\\models\\military.glb",
        "assets/models/military.glb",
        "military.glb",
        "../assets/models/military.glb",
        "../../assets/models/military.glb"};

    bool militaryLoaded = false;
    for (const auto &path : militaryPaths)
    {
        std::cout << "Trying to load model from: " << path << std::endl;
        if (modelManager.loadModel(path, "military"))
        {
            std::cout << "MILITARY model loaded successfully from: " << path << std::endl;
            militaryLoaded = true;
            break;
        }
    }

    if (militaryLoaded)
    {
        glm::mat4 modelTransform = glm::mat4(1.0f);
        modelTransform = glm::translate(modelTransform, militaryPosition);
        modelTransform = glm::scale(modelTransform, glm::vec3(militaryScale));
        militaryInstanceId = modelManager.addModelInstance("military", modelTransform);
        std::cout << "Added MILITARY model instance at (" << militaryPosition.x << ", " << militaryPosition.y << ", " << militaryPosition.z << ") scale " << militaryScale << " - instance ID: " << militaryInstanceId << std::endl;
    }
    else
    {
        std::cerr << "Failed to load MILITARY model from any path" << std::endl;
    }
    */

    // Load the WALKING model - try different paths - DEBUG: USING MILITARY POSITION
    std::vector<std::string> walkingPaths = {
        "assets\\models\\model_Animation_Walking_withSkin.glb",
        "assets/models/model_Animation_Walking_withSkin.glb",
        "model_Animation_Walking_withSkin.glb",
        "../assets/models/model_Animation_Walking_withSkin.glb",
        "../../assets/models/model_Animation_Walking_withSkin.glb"};

    bool walkingLoaded = false;
    for (const auto &path : walkingPaths)
    {
        std::cout << "🚶 DEBUG: Trying to load WALKING model from: " << path << std::endl;
        if (modelManager.loadModel(path, "walking"))
        {
            std::cout << "🚶 SUCCESS: WALKING model loaded successfully from: " << path << std::endl;
            walkingLoaded = true;
            break;
        }
        else
        {
            std::cout << "🚶 FAILED: Could not load WALKING model from: " << path << std::endl;
        }
    }

    if (walkingLoaded)
    {
        glm::mat4 modelTransform = glm::mat4(1.0f);
        // DEBUG: Use military position exactly to replace military model
        modelTransform = glm::translate(modelTransform, militaryPosition);     // Use military position!
        modelTransform = glm::scale(modelTransform, glm::vec3(militaryScale)); // Use military scale!
        walkingInstanceId = modelManager.addModelInstance("walking", modelTransform);
        std::cout << "🚶 SUCCESS: Added WALKING model instance at MILITARY position (" << militaryPosition.x << ", " << militaryPosition.y << ", " << militaryPosition.z << ") scale " << militaryScale << " - instance ID: " << walkingInstanceId << std::endl;

        // DEBUG: Check if model has textures and animations
        GLTFModel *walkingModel = modelManager.getModel("walking");
        if (walkingModel)
        {
            std::cout << "🚶 DEBUG: Walking model loaded with " << walkingModel->getPrimitiveCount() << " primitives" << std::endl;
            std::cout << "🚶 DEBUG: Walking model has " << walkingModel->getVertexCount() << " vertices" << std::endl;

            // FORCE proper bounds for skinned models - the bounds calculation isn't working correctly
            // This is a temporary fix to make the model visible
            std::cout << "🚶 DEBUG: FORCING skinned model bounds to (-1,0,-1) to (1,2,1)" << std::endl;

            glm::vec3 minBounds = glm::vec3(-1.0f, 0.0f, -1.0f);
            glm::vec3 maxBounds = glm::vec3(1.0f, 2.0f, 1.0f);
            glm::vec3 center = (minBounds + maxBounds) * 0.5f;
            std::cout << "🚶 DEBUG: Model bounds: (" << minBounds.x << ", " << minBounds.y << ", " << minBounds.z << ") to (" << maxBounds.x << ", " << maxBounds.y << ", " << maxBounds.z << ")" << std::endl;
            std::cout << "🚶 DEBUG: Model center: (" << center.x << ", " << center.y << ", " << center.z << ")" << std::endl;
        }
    }
    else
    {
        std::cerr << "🚶 ERROR: Failed to load WALKING model from any path!" << std::endl;
        std::cerr << "🚶 Current working directory paths being tried:" << std::endl;
        for (const auto &path : walkingPaths)
        {
            std::cerr << "  - " << path << std::endl;
        }
    }

    bool ok = true;
    // Albedo is color data -> sRGB; roughness/translucency/height are data -> linear
    ok = ok && albedoTex.loadFromFile("snow/snow_02_diff_1k.jpg", true, true);
    ok = ok && roughnessTex.loadFromFile("snow/snow_02_rough_1k.jpg", true, false);
    ok = ok && translucencyTex.loadFromFile("snow/snow_02_translucent_1k.png", true, false);
    ok = ok && heightTex.loadFromFile("snow/snow_02_disp_1k.png", true, false);
    return ok;
}

void Scene::update(const glm::vec3 &cameraPos, float deltaTime, const glm::mat4 &viewMatrix, const glm::mat4 &projectionMatrix)
{
    objectManager.update(cameraPos, cullDistance, 25.0f, 75.0f, deltaTime);

    // Update snow system
    snowSystem.update(deltaTime, cameraPos, viewMatrix, projectionMatrix);

    // Advance simple animation clock
    animElapsed += deltaTime * ((militaryAnimEnabled ? militaryAnimSpeed : 0.0f) + (walkingAnimEnabled ? walkingAnimSpeed : 0.0f));

    // Keep FING instance transform in sync with exposed controls
    if (fingInstanceId >= 0)
    {
        glm::mat4 t = glm::mat4(1.0f);
        t = glm::translate(t, fingPosition);
        t = glm::scale(t, glm::vec3(fingScale));
        modelManager.setInstanceTransform(fingInstanceId, t);
    }

    // Keep MILITARY instance transform in sync with exposed controls - COMMENTED OUT FOR DEBUG
    /*
    if (militaryInstanceId >= 0)
    {
        glm::mat4 t = glm::mat4(1.0f);
        // Subtle idle: vertical bob + slight yaw using animElapsed
        float bob = 0.5f * sin(animElapsed * 2.0f);
        float yaw = 0.15f * sin(animElapsed * 0.7f);
        t = glm::translate(t, militaryPosition + glm::vec3(0.0f, bob, 0.0f));
        t = glm::rotate(t, yaw, glm::vec3(0.0f, 1.0f, 0.0f));
        t = glm::scale(t, glm::vec3(militaryScale));
        modelManager.setInstanceTransform(militaryInstanceId, t);
    }
    */

    // Keep WALKING instance transform in sync with exposed controls
    if (walkingInstanceId >= 0)
    {
        glm::mat4 t = glm::mat4(1.0f);
        // Position it close and visible with reasonable scale for character
        glm::vec3 debugPosition = glm::vec3(0.0f, 2.0f, -5.0f); // 5 units in front, 2 units up to avoid floor clipping
        float debugScale = 10.0f;                               // Reasonable scale for character
        t = glm::translate(t, debugPosition);
        t = glm::scale(t, glm::vec3(debugScale));
        modelManager.setInstanceTransform(walkingInstanceId, t);

        // CRITICAL: Enable animation on the walking model
        GLTFModel *walkingModel = modelManager.getModel("walking");
        if (walkingModel)
        {
            walkingModel->setAnimationEnabled(true);
            walkingModel->setAnimationTime(animElapsed);

            // Debug: Print animation time every 60 frames
            static int animTimeDebugCounter = 0;
            if (++animTimeDebugCounter % 60 == 0)
            {
                std::cout << "🚶 DEBUG: Animation time set to: " << animElapsed << " seconds" << std::endl;
            }
            // Debug: Print bounds every 60 frames (once per second at 60fps)
            static int debugCounter = 0;
            if (++debugCounter % 60 == 0)
            {
                // Use forced bounds for skinned models
                glm::vec3 minBounds = glm::vec3(-1.0f, 0.0f, -1.0f);
                glm::vec3 maxBounds = glm::vec3(1.0f, 2.0f, 1.0f);
                glm::vec3 center = (minBounds + maxBounds) * 0.5f;
                std::cout << "🚶 DEBUG: Model bounds (FORCED): (" << minBounds.x << ", " << minBounds.y << ", " << minBounds.z << ") to (" << maxBounds.x << ", " << maxBounds.y << ", " << maxBounds.z << ")" << std::endl;
                std::cout << "🚶 DEBUG: Model center (FORCED): (" << center.x << ", " << center.y << ", " << center.z << ")" << std::endl;
                std::cout << "🚶 DEBUG: Transform scale: " << debugScale << ", position: (" << debugPosition.x << ", " << debugPosition.y << ", " << debugPosition.z << ") - Y=2.0 to avoid floor clipping" << std::endl;
            }
        }
    }
}

void Scene::render(const glm::mat4 &view, const glm::mat4 &projection, const glm::vec3 &cameraPos, const glm::vec3 &cameraFront, LightManager &lightManager)
{
    // Render floor plane first
    shader.use();

    // Set uniforms for floor
    shader.setUniform("uModel", glm::mat4(1.0f));
    shader.setUniform("uView", view);
    shader.setUniform("uProj", projection);
    shader.setUniform("uLightPos", glm::vec3(2.0f, 4.0f, 2.0f));
    shader.setUniform("uViewPos", cameraPos);
    shader.setUniform("uLightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    shader.setUniform("uObjectColor", glm::vec3(1.0f, 1.0f, 1.0f));
    shader.setUniform("uCullDistance", 400.0f);
    shader.setUniform("uAmbient", ambient);
    shader.setUniform("uSpecularStrength", specularStrength);

    // Set flashlight uniforms for floor
    shader.setUniform("uFlashlightOn", lightManager.isFlashlightOn());
    shader.setUniform("uFlashlightPos", cameraPos);
    shader.setUniform("uFlashlightDir", cameraFront);
    shader.setUniform("uFlashlightCutoff", lightManager.getFlashlightCutoff());
    shader.setUniform("uFlashlightBrightness", lightManager.getFlashlightBrightness());
    shader.setUniform("uFlashlightColor", lightManager.getFlashlightColor());

    // Set fog uniforms for floor
    shader.setUniform("uFogEnabled", fogEnabled);
    shader.setUniform("uFogColor", fogColor);
    shader.setUniform("uFogDensity", fogDensity);
    shader.setUniform("uFogDesaturationStrength", fogDesaturationStrength);
    shader.setUniform("uFogAbsorptionDensity", fogAbsorptionDensity);
    shader.setUniform("uFogAbsorptionStrength", fogAbsorptionStrength);
    shader.setUniform("uBackgroundColor", glm::vec3(0.08f, 0.1f, 0.12f)); // Match renderer clear color

    albedoTex.bind(0);
    shader.setUniform("uAlbedoTex", 0);
    roughnessTex.bind(1);
    shader.setUniform("uRoughnessTex", 1);
    translucencyTex.bind(2);
    shader.setUniform("uTranslucencyTex", 2);
    // Non-tess pipeline: bind height as bump for normal mapping
    heightTex.bind(3);
    shader.setUniform("uHeightTex", 3);
    shader.setUniform("uNormalStrength", normalStrength);
    shader.setUniform("uWorldPerUV", glm::vec2(10.0f, 10.0f));
    shader.setUniform("uRoughnessBias", roughnessBias);

    glBindVertexArray(vao);
    // Draw floor normally (no tessellation)
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);

    // Apply fog settings to ObjectManager
    objectManager.setFogEnabled(fogEnabled);
    objectManager.setFogColor(fogColor);
    objectManager.setFogDensity(fogDensity);
    objectManager.setFogDesaturationStrength(fogDesaturationStrength);
    objectManager.setFogAbsorption(fogAbsorptionDensity, fogAbsorptionStrength);

    // Render objects
    objectManager.render(view, projection, cameraPos, cameraFront, lightManager, albedoTex.getID());

    // Apply fog settings to ModelManager
    modelManager.setFogEnabled(fogEnabled);
    modelManager.setFogColor(fogColor);
    modelManager.setFogDensity(fogDensity);
    modelManager.setFogDesaturationStrength(fogDesaturationStrength);
    modelManager.setFogAbsorption(fogAbsorptionDensity, fogAbsorptionStrength);

    // Render GLTF models
    glm::vec3 lightDir = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.4f));
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    modelManager.render(view, projection, cameraPos, lightDir, lightColor, lightManager);

    // Apply fog settings to SnowSystem
    snowSystem.setFogEnabled(fogEnabled);
    snowSystem.setFogColor(fogColor);
    snowSystem.setFogDensity(fogDensity);
    snowSystem.setFogDesaturationStrength(fogDesaturationStrength);
    snowSystem.setFogAbsorption(fogAbsorptionDensity, fogAbsorptionStrength);

    // Render snow (after opaque objects, before any transparency)
    snowSystem.render(view, projection, cameraPos);
}

void Scene::cleanup()
{
    if (vao != 0)
    {
        glDeleteVertexArrays(1, &vao);
        vao = 0;
    }
    if (vbo != 0)
    {
        glDeleteBuffers(1, &vbo);
        vbo = 0;
    }

    // Cleanup managers
    objectManager.cleanup();
    modelManager.cleanup();
}

void Scene::setupGeometry()
{
    // Floor plane (Y=0) sized 2000x2000 with heavy UV tiling
    float vertices[] = {
        //        position                 normal           uv (tile 200x200)
        -1000.0f,
        0.0f,
        -1000.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        1000.0f,
        0.0f,
        -1000.0f,
        0.0f,
        1.0f,
        0.0f,
        200.0f,
        0.0f,
        1000.0f,
        0.0f,
        1000.0f,
        0.0f,
        1.0f,
        0.0f,
        200.0f,
        200.0f,

        -1000.0f,
        0.0f,
        -1000.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        0.0f,
        1000.0f,
        0.0f,
        1000.0f,
        0.0f,
        1.0f,
        0.0f,
        200.0f,
        200.0f,
        -1000.0f,
        0.0f,
        1000.0f,
        0.0f,
        1.0f,
        0.0f,
        0.0f,
        200.0f,
    };

    glGenVertexArrays(1, &vao);
    glGenBuffers(1, &vbo);

    glBindVertexArray(vao);
    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void *)(6 * sizeof(float)));

    glBindVertexArray(0);
}

void Scene::setupShader()
{
    // Use non-tess pipeline
    shader.loadFromFiles("phong_notess.vert", "phong_notess.frag");
}

void Scene::setObjectCount(int count)
{
    objectManager.setObjectCount(count);
}

int Scene::getObjectCount() const
{
    return objectManager.getObjectCount();
}
