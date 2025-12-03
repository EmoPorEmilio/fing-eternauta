#include "LightManager.h"
#include <iostream>
#include <string>

LightManager::LightManager() : flashlightIndex(-1), flashlightEnabled(false)
{
    // Initialize with default lights
    addDirectionalLight(glm::vec3(0.0f, -1.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), 0.8f);
    addPointLight(glm::vec3(0.0f, 10.0f, 0.0f), glm::vec3(1.0f, 1.0f, 1.0f), 0.6f);

    // Initialize flashlight (disabled by default)
    setFlashlight(glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, -1.0f), false);

    // Subscribe to events
    subscribeToEvents();

    std::cout << "LightManager initialized with " << lights.size() << " lights, flashlight index: " << flashlightIndex << std::endl;
}

LightManager::~LightManager()
{
    unsubscribeFromEvents();
    clearLights();
}

void LightManager::addLight(const Light &light)
{
    lights.push_back(light);

    // Create ECS entity for this light
    ecs::Entity entity = createLightEntity(light);
    m_lightEntities.push_back(entity);
}

void LightManager::removeLight(size_t index)
{
    if (index < lights.size())
    {
        // Destroy the ECS entity
        if (index < m_lightEntities.size())
        {
            auto& registry = ECSWorld::getRegistry();
            if (registry.isValid(m_lightEntities[index]))
            {
                registry.destroy(m_lightEntities[index]);
            }
            m_lightEntities.erase(m_lightEntities.begin() + index);
        }

        lights.erase(lights.begin() + index);
        if (flashlightIndex >= 0)
        {
            if (index < static_cast<size_t>(flashlightIndex))
            {
                flashlightIndex--;
            }
            else if (index == static_cast<size_t>(flashlightIndex))
            {
                flashlightIndex = -1; // Reset if we removed the flashlight
                m_flashlightEntity = ecs::Entity();
            }
        }
    }
}

void LightManager::clearLights()
{
    // Destroy all ECS entities
    auto& registry = ECSWorld::getRegistry();
    for (auto entity : m_lightEntities)
    {
        if (registry.isValid(entity))
        {
            registry.destroy(entity);
        }
    }
    m_lightEntities.clear();
    m_flashlightEntity = ecs::Entity();

    lights.clear();
    flashlightIndex = -1;
}

void LightManager::addDirectionalLight(const glm::vec3 &direction, const glm::vec3 &color, float intensity)
{
    Light light;
    light.type = LightType::DIRECTIONAL;
    light.direction = glm::normalize(direction);
    light.color = color;
    light.intensity = intensity;
    light.enabled = true;
    addLight(light);
}

void LightManager::addPointLight(const glm::vec3 &position, const glm::vec3 &color, float intensity)
{
    Light light;
    light.type = LightType::POINT;
    light.position = position;
    light.color = color;
    light.intensity = intensity;
    light.enabled = true;
    addLight(light);
}

void LightManager::addSpotlight(const glm::vec3 &position, const glm::vec3 &direction, float cutoff,
                                const glm::vec3 &color, float intensity)
{
    Light light;
    light.type = LightType::SPOTLIGHT;
    light.position = position;
    light.direction = glm::normalize(direction);
    light.color = color;
    light.intensity = intensity;
    light.cutoff = cutoff;
    light.outerCutoff = cutoff * 0.9f; // Smooth falloff
    light.enabled = true;
    addLight(light);
}

void LightManager::setFlashlight(const glm::vec3 &position, const glm::vec3 &direction, bool enabled)
{
    flashlightEnabled = enabled;
    auto& registry = ECSWorld::getRegistry();

    if (flashlightIndex >= 0 && flashlightIndex < static_cast<int>(lights.size()))
    {
        // Update existing flashlight
        lights[flashlightIndex].position = position;
        lights[flashlightIndex].direction = glm::normalize(direction);
        lights[flashlightIndex].enabled = enabled;
        lights[flashlightIndex].color = glm::vec3(1.0f, 0.8f, 0.6f); // Warm light
        lights[flashlightIndex].intensity = 3.0f;
        lights[flashlightIndex].cutoff = glm::cos(glm::radians(25.0f));
        lights[flashlightIndex].outerCutoff = glm::cos(glm::radians(30.0f));

        // Update ECS entity
        if (m_flashlightEntity.isValid() && registry.isValid(m_flashlightEntity))
        {
            if (auto* transform = registry.tryGet<ecs::TransformComponent>(m_flashlightEntity))
            {
                transform->position = position;
            }
            if (auto* lightComp = registry.tryGet<ecs::LightComponent>(m_flashlightEntity))
            {
                lightComp->direction = glm::normalize(direction);
                lightComp->enabled = enabled;
                lightComp->color = glm::vec3(1.0f, 0.8f, 0.6f);
                lightComp->intensity = 3.0f;
                lightComp->cutoff = glm::cos(glm::radians(25.0f));
                lightComp->outerCutoff = glm::cos(glm::radians(30.0f));
            }
        }
    }
    else
    {
        // Create new flashlight
        addSpotlight(position, direction, glm::cos(glm::radians(25.0f)),
                     glm::vec3(1.0f, 0.8f, 0.6f), 3.0f);
        flashlightIndex = static_cast<int>(lights.size()) - 1;
        lights[flashlightIndex].enabled = enabled;

        // Track flashlight entity
        if (!m_lightEntities.empty())
        {
            m_flashlightEntity = m_lightEntities.back();
            // Update enabled state in ECS
            if (auto* lightComp = registry.tryGet<ecs::LightComponent>(m_flashlightEntity))
            {
                lightComp->enabled = enabled;
            }
        }
    }
}

void LightManager::updateFlashlight(const glm::vec3 &position, const glm::vec3 &direction)
{
    if (flashlightIndex >= 0 && flashlightIndex < static_cast<int>(lights.size()))
    {
        lights[flashlightIndex].position = position;
        lights[flashlightIndex].direction = glm::normalize(direction);

        // Update ECS entity
        auto& registry = ECSWorld::getRegistry();
        if (m_flashlightEntity.isValid() && registry.isValid(m_flashlightEntity))
        {
            if (auto* transform = registry.tryGet<ecs::TransformComponent>(m_flashlightEntity))
            {
                transform->position = position;
            }
            if (auto* lightComp = registry.tryGet<ecs::LightComponent>(m_flashlightEntity))
            {
                lightComp->direction = glm::normalize(direction);
            }
        }
    }
}

void LightManager::toggleFlashlight()
{
    flashlightEnabled = !flashlightEnabled;
    if (flashlightIndex >= 0 && flashlightIndex < static_cast<int>(lights.size()))
    {
        lights[flashlightIndex].enabled = flashlightEnabled;

        // Update ECS entity
        auto& registry = ECSWorld::getRegistry();
        if (m_flashlightEntity.isValid() && registry.isValid(m_flashlightEntity))
        {
            if (auto* lightComp = registry.tryGet<ecs::LightComponent>(m_flashlightEntity))
            {
                lightComp->enabled = flashlightEnabled;
            }
        }
    }
    std::cout << "Flashlight: " << (flashlightEnabled ? "ON" : "OFF") << " (State: " << flashlightEnabled << ")" << std::endl;
    updateFlashlightUBO();
}

const LightManager::LightUniformCache& LightManager::getCachedUniforms(GLuint shaderProgram) const
{
    auto it = m_uniformCache.find(shaderProgram);
    if (it != m_uniformCache.end())
    {
        return it->second;
    }

    // Cache miss - build the cache for this shader
    LightUniformCache cache;
    cache.numLights = glGetUniformLocation(shaderProgram, "uNumLights");
    cache.viewPos = glGetUniformLocation(shaderProgram, "uViewPos");

    for (int i = 0; i < MAX_CACHED_LIGHTS; ++i)
    {
        std::string prefix = "uLights[" + std::to_string(i) + "].";
        cache.lights[i].type = glGetUniformLocation(shaderProgram, (prefix + "type").c_str());
        cache.lights[i].position = glGetUniformLocation(shaderProgram, (prefix + "position").c_str());
        cache.lights[i].direction = glGetUniformLocation(shaderProgram, (prefix + "direction").c_str());
        cache.lights[i].color = glGetUniformLocation(shaderProgram, (prefix + "color").c_str());
        cache.lights[i].intensity = glGetUniformLocation(shaderProgram, (prefix + "intensity").c_str());
        cache.lights[i].cutoff = glGetUniformLocation(shaderProgram, (prefix + "cutoff").c_str());
        cache.lights[i].outerCutoff = glGetUniformLocation(shaderProgram, (prefix + "outerCutoff").c_str());
        cache.lights[i].constant = glGetUniformLocation(shaderProgram, (prefix + "constant").c_str());
        cache.lights[i].linear = glGetUniformLocation(shaderProgram, (prefix + "linear").c_str());
        cache.lights[i].quadratic = glGetUniformLocation(shaderProgram, (prefix + "quadratic").c_str());
        cache.lights[i].enabled = glGetUniformLocation(shaderProgram, (prefix + "enabled").c_str());
    }

    m_uniformCache[shaderProgram] = cache;
    return m_uniformCache[shaderProgram];
}

void LightManager::applyLightsToShader(GLuint shaderProgram, const glm::vec3 &cameraPos) const
{
    // Get cached uniform locations (builds cache on first call per shader)
    const LightUniformCache& cache = getCachedUniforms(shaderProgram);

    int lightCount = 0;

    // Gather lights from ECS entities
    auto& registry = ECSWorld::getRegistry();
    registry.each<ecs::TransformComponent, ecs::LightComponent>(
        [&](ecs::Entity entity, ecs::TransformComponent& transform, ecs::LightComponent& lightComp) {
            if (lightCount >= MAX_CACHED_LIGHTS) return;

            const auto& loc = cache.lights[lightCount];

            if (loc.type != -1)
                glUniform1i(loc.type, static_cast<int>(lightComp.type));
            if (loc.position != -1)
                glUniform3f(loc.position, transform.position.x, transform.position.y, transform.position.z);
            if (loc.direction != -1)
                glUniform3f(loc.direction, lightComp.direction.x, lightComp.direction.y, lightComp.direction.z);
            if (loc.color != -1)
                glUniform3f(loc.color, lightComp.color.x, lightComp.color.y, lightComp.color.z);
            if (loc.intensity != -1)
                glUniform1f(loc.intensity, lightComp.intensity);
            if (loc.cutoff != -1)
                glUniform1f(loc.cutoff, lightComp.cutoff);
            if (loc.outerCutoff != -1)
                glUniform1f(loc.outerCutoff, lightComp.outerCutoff);
            if (loc.constant != -1)
                glUniform1f(loc.constant, lightComp.constant);
            if (loc.linear != -1)
                glUniform1f(loc.linear, lightComp.linear);
            if (loc.quadratic != -1)
                glUniform1f(loc.quadratic, lightComp.quadratic);
            if (loc.enabled != -1)
                glUniform1i(loc.enabled, lightComp.enabled ? 1 : 0);

            lightCount++;
        });

    // Set number of lights using cached location
    if (cache.numLights != -1)
    {
        glUniform1i(cache.numLights, lightCount);
    }

    // Set camera position for lighting calculations using cached location
    if (cache.viewPos != -1)
    {
        glUniform3f(cache.viewPos, cameraPos.x, cameraPos.y, cameraPos.z);
    }
}

glm::vec3 LightManager::getFlashlightPosition() const
{
    if (flashlightIndex >= 0 && flashlightIndex < static_cast<int>(lights.size()))
    {
        return lights[flashlightIndex].position;
    }
    return glm::vec3(0.0f);
}

glm::vec3 LightManager::getFlashlightDirection() const
{
    if (flashlightIndex >= 0 && flashlightIndex < static_cast<int>(lights.size()))
    {
        return lights[flashlightIndex].direction;
    }
    return glm::vec3(0.0f, 0.0f, -1.0f);
}

float LightManager::getFlashlightCutoff() const
{
    if (flashlightIndex >= 0 && flashlightIndex < static_cast<int>(lights.size()))
    {
        return lights[flashlightIndex].cutoff;
    }
    return cos(glm::radians(25.0f));
}

float LightManager::getFlashlightOuterCutoff() const
{
    if (flashlightIndex >= 0 && flashlightIndex < static_cast<int>(lights.size()))
    {
        return lights[flashlightIndex].outerCutoff;
    }
    return cos(glm::radians(30.0f));
}

void LightManager::initializeGLResources()
{
    if (flashlightUBO == 0)
    {
        glGenBuffers(1, &flashlightUBO);
        glBindBuffer(GL_UNIFORM_BUFFER, flashlightUBO);
        // struct layout: vec4 pos, vec4 dir, vec4 color, vec4 params(enabled, cutoff, outerCutoff, brightness)
        glBufferData(GL_UNIFORM_BUFFER, sizeof(float) * 16, nullptr, GL_DYNAMIC_DRAW);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
        glBindBufferBase(GL_UNIFORM_BUFFER, 2, flashlightUBO);
        updateFlashlightUBO();
    }
}

void LightManager::updateFlashlightUBO() const
{
    if (flashlightUBO == 0)
        return;

    glm::vec3 pos = getFlashlightPosition();
    glm::vec3 dir = getFlashlightDirection();
    glm::vec3 col = getFlashlightColor();
    float enabled = isFlashlightOn() ? 1.0f : 0.0f;
    float cutoff = getFlashlightCutoff();
    float outer = getFlashlightOuterCutoff();
    float brightness = getFlashlightBrightness();

    struct Block
    {
        glm::vec4 p;
        glm::vec4 d;
        glm::vec4 c;
        glm::vec4 params;
    } data;
    data.p = glm::vec4(pos, 1.0f);
    data.d = glm::vec4(glm::normalize(dir), 0.0f);
    data.c = glm::vec4(col, 1.0f);
    data.params = glm::vec4(enabled, cutoff, outer, brightness);

    glBindBuffer(GL_UNIFORM_BUFFER, flashlightUBO);
    glBufferSubData(GL_UNIFORM_BUFFER, 0, sizeof(data), &data);
    glBindBuffer(GL_UNIFORM_BUFFER, 0);
}

void LightManager::setFlashlightBrightness(float brightness)
{
    if (flashlightIndex >= 0 && flashlightIndex < static_cast<int>(lights.size()))
    {
        lights[flashlightIndex].intensity = brightness;

        // Update ECS entity
        auto& registry = ECSWorld::getRegistry();
        if (m_flashlightEntity.isValid() && registry.isValid(m_flashlightEntity))
        {
            if (auto* lightComp = registry.tryGet<ecs::LightComponent>(m_flashlightEntity))
            {
                lightComp->intensity = brightness;
            }
        }
    }
    else
    {
        std::cout << "ERROR: Flashlight index " << flashlightIndex << " out of range (lights.size(): " << lights.size() << ")" << std::endl;
    }
}

void LightManager::setFlashlightColor(const glm::vec3 &color)
{
    if (flashlightIndex >= 0 && flashlightIndex < static_cast<int>(lights.size()))
    {
        lights[flashlightIndex].color = color;

        // Update ECS entity
        auto& registry = ECSWorld::getRegistry();
        if (m_flashlightEntity.isValid() && registry.isValid(m_flashlightEntity))
        {
            if (auto* lightComp = registry.tryGet<ecs::LightComponent>(m_flashlightEntity))
            {
                lightComp->color = color;
            }
        }
    }
}

void LightManager::setFlashlightCutoff(float cutoffDegrees)
{
    if (flashlightIndex >= 0 && flashlightIndex < static_cast<int>(lights.size()))
    {
        lights[flashlightIndex].cutoff = glm::cos(glm::radians(cutoffDegrees));
        lights[flashlightIndex].outerCutoff = glm::cos(glm::radians(cutoffDegrees * 0.9f));

        // Update ECS entity
        auto& registry = ECSWorld::getRegistry();
        if (m_flashlightEntity.isValid() && registry.isValid(m_flashlightEntity))
        {
            if (auto* lightComp = registry.tryGet<ecs::LightComponent>(m_flashlightEntity))
            {
                lightComp->cutoff = glm::cos(glm::radians(cutoffDegrees));
                lightComp->outerCutoff = glm::cos(glm::radians(cutoffDegrees * 0.9f));
            }
        }
    }
    else
    {
        std::cout << "ERROR: Flashlight index " << flashlightIndex << " out of range (lights.size(): " << lights.size() << ")" << std::endl;
    }
}

float LightManager::getFlashlightBrightness() const
{
    if (flashlightIndex >= 0 && flashlightIndex < static_cast<int>(lights.size()))
    {
        return lights[flashlightIndex].intensity;
    }
    return 1.0f;
}

glm::vec3 LightManager::getFlashlightColor() const
{
    if (flashlightIndex >= 0 && flashlightIndex < static_cast<int>(lights.size()))
    {
        return lights[flashlightIndex].color;
    }
    return glm::vec3(1.0f, 0.8f, 0.6f);
}

// =========== ECS Implementation ===========

ecs::Entity LightManager::createLightEntity(const Light& light)
{
    auto& registry = ECSWorld::getRegistry();
    ecs::Entity entity = registry.create();

    // Add TransformComponent
    auto& transform = registry.add<ecs::TransformComponent>(entity);
    transform.position = light.position;
    transform.dirty = true;

    // Add LightComponent
    auto& lightComp = registry.add<ecs::LightComponent>(entity);

    // Convert legacy LightType to ECS LightType
    switch (light.type)
    {
        case LightType::DIRECTIONAL:
            lightComp.type = ecs::LightType::DIRECTIONAL;
            break;
        case LightType::POINT:
            lightComp.type = ecs::LightType::POINT;
            break;
        case LightType::SPOTLIGHT:
            lightComp.type = ecs::LightType::SPOTLIGHT;
            break;
    }

    lightComp.direction = light.direction;
    lightComp.color = light.color;
    lightComp.intensity = light.intensity;
    lightComp.cutoff = light.cutoff;
    lightComp.outerCutoff = light.outerCutoff;
    lightComp.constant = light.constant;
    lightComp.linear = light.linear;
    lightComp.quadratic = light.quadratic;
    lightComp.enabled = light.enabled;

    return entity;
}

// =========== Event Handling ===========

void LightManager::subscribeToEvents()
{
    auto& bus = events::EventBus::instance();

    // Subscribe to flashlight toggle events from InputManager
    m_flashlightToggleSubscription = bus.subscribe<events::FlashlightToggleEvent>(
        this, &LightManager::onFlashlightToggle);

    // Subscribe to flashlight config changes (brightness, color, cutoff)
    m_flashlightConfigSubscription = bus.subscribe<events::FlashlightConfigChangedEvent>(
        this, &LightManager::onFlashlightConfigChanged);

    std::cout << "[LightManager] Subscribed to flashlight events" << std::endl;
}

void LightManager::unsubscribeFromEvents()
{
    auto& bus = events::EventBus::instance();

    if (m_flashlightToggleSubscription != 0)
    {
        bus.unsubscribe(m_flashlightToggleSubscription);
        m_flashlightToggleSubscription = 0;
    }

    if (m_flashlightConfigSubscription != 0)
    {
        bus.unsubscribe(m_flashlightConfigSubscription);
        m_flashlightConfigSubscription = 0;
    }
}

void LightManager::onFlashlightToggle(const events::FlashlightToggleEvent& event)
{
    // Toggle flashlight when event is received
    toggleFlashlight();
}

void LightManager::onFlashlightConfigChanged(const events::FlashlightConfigChangedEvent& event)
{
    // Update flashlight settings from event
    if (flashlightIndex >= 0 && flashlightIndex < static_cast<int>(lights.size()))
    {
        lights[flashlightIndex].color = event.color;
        lights[flashlightIndex].intensity = event.brightness;
        lights[flashlightIndex].cutoff = glm::cos(glm::radians(event.cutoffDegrees));
        lights[flashlightIndex].outerCutoff = glm::cos(glm::radians(event.cutoffDegrees * 1.1f));

        // Update ECS component if entity exists
        if (m_flashlightEntity.isValid())
        {
            auto& registry = ECSWorld::getRegistry();
            if (registry.has<ecs::LightComponent>(m_flashlightEntity))
            {
                auto& comp = registry.get<ecs::LightComponent>(m_flashlightEntity);
                comp.color = event.color;
                comp.intensity = event.brightness;
                comp.cutoff = lights[flashlightIndex].cutoff;
                comp.outerCutoff = lights[flashlightIndex].outerCutoff;
            }
        }

        // Update flashlight UBO
        updateFlashlightUBO();
    }
}
