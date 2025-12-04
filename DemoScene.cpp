#include "DemoScene.h"
#include "LightManager.h"
#include "events/EventBus.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

DemoScene::DemoScene()
{
}

DemoScene::~DemoScene()
{
    if (m_modelsSubscription != 0) {
        events::EventBus::instance().unsubscribe(m_modelsSubscription);
    }
    cleanup();
}

bool DemoScene::initialize()
{
    // Initialize base scene (floor, fog)
    if (!BaseScene::initialize()) {
        return false;
    }

    // Subscribe to model config changes
    m_modelsSubscription = events::EventBus::instance().subscribe<events::ModelsConfigChangedEvent>(
        [this](const events::ModelsConfigChangedEvent& e) { onModelsConfigChanged(e); }
    );

    // Initialize ObjectManager with default object count
    std::cout << "Initializing ObjectManager..." << std::endl;
    m_objectManager.initialize(ObjectManager::PRESET_MEDIUM);
    std::cout << "ObjectManager initialized with " << m_objectManager.getObjectCount() << " objects" << std::endl;

    // Initialize SnowSystem
    std::cout << "Initializing SnowSystem..." << std::endl;
    if (!m_snowSystem.initialize()) {
        std::cerr << "WARNING: Failed to initialize SnowSystem" << std::endl;
        // Continue anyway - snow is optional
    } else {
        std::cout << "SnowSystem initialized" << std::endl;
    }

    // Initialize model manager
    if (!m_modelManager.initialize()) {
        std::cerr << "Failed to initialize ModelManager" << std::endl;
        return false;
    }

    // Load the first WALKING model (model_Animation_Walking_withSkin.glb)
    std::vector<std::string> walkingPaths = {
        "assets\\models\\model_Animation_Walking_withSkin.glb",
        "assets/models/model_Animation_Walking_withSkin.glb"
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
        modelTransform = glm::translate(modelTransform, m_walkingPosition);
        modelTransform = glm::scale(modelTransform, glm::vec3(m_walkingScale));
        m_walkingInstanceId = m_modelManager.addModelInstance("walking", modelTransform);
        std::cout << "Added WALKING model instance - ID: " << m_walkingInstanceId << std::endl;
    }

    // Load the second model (monster-2.glb)
    std::vector<std::string> monster2Paths = {
        "assets\\models\\monster-2.glb",
        "assets/models/monster-2.glb"
    };

    bool monster2Loaded = false;
    for (const auto& path : monster2Paths) {
        if (m_modelManager.loadModel(path, "monster2")) {
            std::cout << "MONSTER-2 model loaded successfully from: " << path << std::endl;
            monster2Loaded = true;
            break;
        }
    }

    if (monster2Loaded) {
        glm::mat4 modelTransform = glm::mat4(1.0f);
        modelTransform = glm::translate(modelTransform, m_monster2Position);
        modelTransform = glm::scale(modelTransform, glm::vec3(m_monster2Scale));
        m_monster2InstanceId = m_modelManager.addModelInstance("monster2", modelTransform);
        std::cout << "Added MONSTER-2 model instance - ID: " << m_monster2InstanceId << std::endl;
    }

    return true;
}

void DemoScene::update(const glm::vec3& cameraPos, float deltaTime,
                       const glm::mat4& viewMatrix, const glm::mat4& projectionMatrix)
{
    // Update ObjectManager (culling and LOD systems)
    m_objectManager.update(cameraPos, m_cullDistance, 50.0f, 150.0f, deltaTime);

    // Update SnowSystem
    m_snowSystem.update(deltaTime, cameraPos, viewMatrix, projectionMatrix);

    // Update WALKING model visibility and transform
    if (m_walkingInstanceId >= 0) {
        m_modelManager.setInstanceVisibility(m_walkingInstanceId, m_walkingEnabled);

        if (m_walkingEnabled) {
            glm::mat4 t = glm::mat4(1.0f);
            t = glm::translate(t, m_walkingPosition);
            t = glm::scale(t, glm::vec3(m_walkingScale));
            m_modelManager.setInstanceTransform(m_walkingInstanceId, t);

            GLTFModel* walkingModel = m_modelManager.getModel("walking");
            if (walkingModel) {
                walkingModel->setAnimationEnabled(m_walkingAnimEnabled);
                if (m_walkingAnimEnabled) {
                    walkingModel->advanceAnimation(deltaTime * m_walkingAnimSpeed);
                }
            }
        }
    }

    // Update MONSTER-2 model visibility and transform
    if (m_monster2InstanceId >= 0) {
        m_modelManager.setInstanceVisibility(m_monster2InstanceId, m_monster2Enabled);

        if (m_monster2Enabled) {
            glm::mat4 t = glm::mat4(1.0f);
            t = glm::translate(t, m_monster2Position);
            t = glm::scale(t, glm::vec3(m_monster2Scale));
            m_modelManager.setInstanceTransform(m_monster2InstanceId, t);

            GLTFModel* monster2Model = m_modelManager.getModel("monster2");
            if (monster2Model) {
                monster2Model->setAnimationEnabled(m_monster2AnimEnabled);
                if (m_monster2AnimEnabled) {
                    monster2Model->advanceAnimation(deltaTime * m_monster2AnimSpeed);
                }
            }
        }
    }
}

void DemoScene::render(const glm::mat4& view, const glm::mat4& projection,
                       const glm::vec3& cameraPos, const glm::vec3& cameraFront,
                       LightManager& lightManager)
{
    // First render base scene (floor and debug visualization)
    BaseScene::render(view, projection, cameraPos, cameraFront, lightManager);

    // Render instanced prism objects
    m_objectManager.render(view, projection, cameraPos, cameraFront, lightManager, 0);

    // Render GLTF models with fog disabled for now
    m_modelManager.setFogEnabled(false);
    glm::vec3 lightDir = glm::normalize(glm::vec3(-0.3f, -1.0f, -0.4f));
    glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
    m_modelManager.render(view, projection, cameraPos, lightDir, lightColor, lightManager);

    // Render snow particles last (after opaque geometry)
    m_snowSystem.render(view, projection, cameraPos);
}

void DemoScene::cleanup()
{
    if (m_modelsSubscription != 0) {
        events::EventBus::instance().unsubscribe(m_modelsSubscription);
        m_modelsSubscription = 0;
    }
    m_snowSystem.shutdown();
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

void DemoScene::onModelsConfigChanged(const events::ModelsConfigChangedEvent& event)
{
    // Update walking model config
    m_walkingEnabled = event.walking.enabled;
    m_walkingPosition = event.walking.position;
    m_walkingScale = event.walking.scale;
    m_walkingAnimEnabled = event.walking.animationEnabled;
    m_walkingAnimSpeed = event.walking.animationSpeed;

    // Update monster-2 config
    m_monster2Enabled = event.monster2.enabled;
    m_monster2Position = event.monster2.position;
    m_monster2Scale = event.monster2.scale;
    m_monster2AnimEnabled = event.monster2.animationEnabled;
    m_monster2AnimSpeed = event.monster2.animationSpeed;
}
