#include "DemoScene.h"
#include "LightManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

DemoScene::DemoScene()
{
}

DemoScene::~DemoScene()
{
    cleanup();
}

bool DemoScene::initialize()
{
    // Initialize base scene (floor, fog)
    if (!BaseScene::initialize()) {
        return false;
    }

    // MINIMAL DEBUG MODE: Skip ObjectManager and SnowSystem entirely
    // m_objectManager.initialize();  // SKIP - too slow for debugging

    // Initialize model manager
    if (!m_modelManager.initialize()) {
        std::cerr << "Failed to initialize ModelManager" << std::endl;
        return false;
    }

    // Skip snow for debugging
    // m_snowSystem.initialize();

    // Skip FING model - only load walking model

    // Load the WALKING model
    std::vector<std::string> walkingPaths = {
        "assets\\models\\model_Animation_Walking_withSkin.glb",
        "assets/models/model_Animation_Walking_withSkin.glb",
        "model_Animation_Walking_withSkin.glb",
        "../assets/models/model_Animation_Walking_withSkin.glb",
        "../../assets/models/model_Animation_Walking_withSkin.glb"
    };

    bool walkingLoaded = false;
    for (const auto& path : walkingPaths) {
        if (m_modelManager.loadModel(path, "walking")) {
            std::cout << "WALKING model loaded successfully from: " << path << std::endl;
            walkingLoaded = true;
            break;
        }
    }

    if (walkingLoaded) {
        glm::mat4 modelTransform = glm::mat4(1.0f);
        modelTransform = glm::translate(modelTransform, m_militaryPosition);
        modelTransform = glm::scale(modelTransform, glm::vec3(m_militaryScale));
        m_walkingInstanceId = m_modelManager.addModelInstance("walking", modelTransform);
        std::cout << "Added WALKING model instance - ID: " << m_walkingInstanceId << std::endl;
    }

    return true;
}

void DemoScene::update(const glm::vec3& cameraPos, float deltaTime,
                       const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
    // MINIMAL DEBUG MODE - only update walking model

    // Update WALKING instance transform - positioned in front of camera
    if (m_walkingInstanceId >= 0) {
        glm::mat4 t = glm::mat4(1.0f);
        // Position at origin, scale up since vertices are ~0.0003 units
        glm::vec3 debugPosition = glm::vec3(0.0f, 0.0f, -5.0f);
        float debugScale = 1000.0f;  // 1/3 of previous scale
        t = glm::translate(t, debugPosition);
        t = glm::scale(t, glm::vec3(debugScale));
        m_modelManager.setInstanceTransform(m_walkingInstanceId, t);

        // Enable animation
        GLTFModel* walkingModel = m_modelManager.getModel("walking");
        if (walkingModel) {
            walkingModel->setAnimationEnabled(true);
            walkingModel->advanceAnimation(deltaTime);
        }
    }
}

void DemoScene::render(const glm::mat4& view, const glm::mat4& projection,
                       const glm::vec3& cameraPos, const glm::vec3& cameraFront,
                       LightManager& lightManager)
{
    // DEBUG MODE: Only render the walking model, skip everything else
    // Disable fog for clear visibility
    m_modelManager.setFogEnabled(false);

    // Render ONLY the GLTF models (walking model)
    glm::vec3 lightDir = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.4f));
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    m_modelManager.render(view, projection, cameraPos, lightDir, lightColor, lightManager);

    // Skip floor, objects, snow for now
}

void DemoScene::cleanup()
{
    m_objectManager.cleanup();
    m_modelManager.cleanup();
    BaseScene::cleanup();
}

void DemoScene::setObjectCount(int count)
{
    m_objectManager.setObjectCount(count);
}

int DemoScene::getObjectCount() const
{
    return m_objectManager.getObjectCount();
}
