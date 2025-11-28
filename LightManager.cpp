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

    std::cout << "LightManager initialized with " << lights.size() << " lights, flashlight index: " << flashlightIndex << std::endl;
}

LightManager::~LightManager()
{
    clearLights();
}

void LightManager::addLight(const Light &light)
{
    lights.push_back(light);
}

void LightManager::removeLight(size_t index)
{
    if (index < lights.size())
    {
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
            }
        }
    }
}

void LightManager::clearLights()
{
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
    lights.push_back(light);
}

void LightManager::addPointLight(const glm::vec3 &position, const glm::vec3 &color, float intensity)
{
    Light light;
    light.type = LightType::POINT;
    light.position = position;
    light.color = color;
    light.intensity = intensity;
    light.enabled = true;
    lights.push_back(light);
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
    lights.push_back(light);
}

void LightManager::setFlashlight(const glm::vec3 &position, const glm::vec3 &direction, bool enabled)
{
    flashlightEnabled = enabled;

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
    }
    else
    {
        // Create new flashlight
        addSpotlight(position, direction, glm::cos(glm::radians(25.0f)),
                     glm::vec3(1.0f, 0.8f, 0.6f), 3.0f);
        flashlightIndex = static_cast<int>(lights.size()) - 1;
        lights[flashlightIndex].enabled = enabled;
    }
}

void LightManager::updateFlashlight(const glm::vec3 &position, const glm::vec3 &direction)
{
    if (flashlightIndex >= 0 && flashlightIndex < static_cast<int>(lights.size()))
    {
        lights[flashlightIndex].position = position;
        lights[flashlightIndex].direction = glm::normalize(direction);
    }
}

void LightManager::toggleFlashlight()
{
    flashlightEnabled = !flashlightEnabled;
    if (flashlightIndex >= 0 && flashlightIndex < static_cast<int>(lights.size()))
    {
        lights[flashlightIndex].enabled = flashlightEnabled;
    }
    std::cout << "Flashlight: " << (flashlightEnabled ? "ON" : "OFF") << " (State: " << flashlightEnabled << ")" << std::endl;
    updateFlashlightUBO();
}

void LightManager::applyLightsToShader(GLuint shaderProgram, const glm::vec3 &cameraPos) const
{
    // Set number of lights
    GLint numLightsLoc = glGetUniformLocation(shaderProgram, "uNumLights");
    if (numLightsLoc != -1)
    {
        glUniform1i(numLightsLoc, static_cast<int>(lights.size()));
    }

    // Apply each light
    for (size_t i = 0; i < lights.size(); ++i)
    {
        const Light &light = lights[i];
        std::string prefix = "uLights[" + std::to_string(i) + "].";

        GLint typeLoc = glGetUniformLocation(shaderProgram, (prefix + "type").c_str());
        GLint positionLoc = glGetUniformLocation(shaderProgram, (prefix + "position").c_str());
        GLint directionLoc = glGetUniformLocation(shaderProgram, (prefix + "direction").c_str());
        GLint colorLoc = glGetUniformLocation(shaderProgram, (prefix + "color").c_str());
        GLint intensityLoc = glGetUniformLocation(shaderProgram, (prefix + "intensity").c_str());
        GLint cutoffLoc = glGetUniformLocation(shaderProgram, (prefix + "cutoff").c_str());
        GLint outerCutoffLoc = glGetUniformLocation(shaderProgram, (prefix + "outerCutoff").c_str());
        GLint constantLoc = glGetUniformLocation(shaderProgram, (prefix + "constant").c_str());
        GLint linearLoc = glGetUniformLocation(shaderProgram, (prefix + "linear").c_str());
        GLint quadraticLoc = glGetUniformLocation(shaderProgram, (prefix + "quadratic").c_str());
        GLint enabledLoc = glGetUniformLocation(shaderProgram, (prefix + "enabled").c_str());

        if (typeLoc != -1)
            glUniform1i(typeLoc, static_cast<int>(light.type));
        if (positionLoc != -1)
            glUniform3f(positionLoc, light.position.x, light.position.y, light.position.z);
        if (directionLoc != -1)
            glUniform3f(directionLoc, light.direction.x, light.direction.y, light.direction.z);
        if (colorLoc != -1)
            glUniform3f(colorLoc, light.color.x, light.color.y, light.color.z);
        if (intensityLoc != -1)
            glUniform1f(intensityLoc, light.intensity);
        if (cutoffLoc != -1)
            glUniform1f(cutoffLoc, light.cutoff);
        if (outerCutoffLoc != -1)
            glUniform1f(outerCutoffLoc, light.outerCutoff);
        if (constantLoc != -1)
            glUniform1f(constantLoc, light.constant);
        if (linearLoc != -1)
            glUniform1f(linearLoc, light.linear);
        if (quadraticLoc != -1)
            glUniform1f(quadraticLoc, light.quadratic);
        if (enabledLoc != -1)
            glUniform1i(enabledLoc, light.enabled ? 1 : 0);
    }

    // Set camera position for lighting calculations
    GLint viewPosLoc = glGetUniformLocation(shaderProgram, "uViewPos");
    if (viewPosLoc != -1)
    {
        glUniform3f(viewPosLoc, cameraPos.x, cameraPos.y, cameraPos.z);
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
    }
}

void LightManager::setFlashlightCutoff(float cutoffDegrees)
{
    if (flashlightIndex >= 0 && flashlightIndex < static_cast<int>(lights.size()))
    {
        lights[flashlightIndex].cutoff = glm::cos(glm::radians(cutoffDegrees));
        lights[flashlightIndex].outerCutoff = glm::cos(glm::radians(cutoffDegrees * 0.9f));
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