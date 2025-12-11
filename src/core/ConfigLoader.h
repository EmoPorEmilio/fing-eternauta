#pragma once
#include <string>
#include <iostream>
#include <sstream>
#include <glm/glm.hpp>
#include "tinyxml.h"

// Runtime-configurable game settings loaded from config.xml
// Values are initialized with defaults matching the original GameConfig constants
struct GameSettings {
    // Window
    int windowWidth = 1280;
    int windowHeight = 720;
    bool windowFullscreen = false;
    std::string windowTitle = "fing-eternauta";

    // Graphics
    int shadowMapSize = 2048;
    float shadowOrthoSize = 100.0f;
    float shadowNear = 1.0f;
    float shadowFar = 200.0f;
    float shadowDistance = 80.0f;

    // Fog
    float fogDensity = 0.02f;
    float fogDesaturation = 0.8f;
    glm::vec3 fogColor = glm::vec3(0.15f, 0.15f, 0.17f);

    // Player
    float playerMoveSpeed = 3.0f;
    float playerTurnSpeed = 10.0f;
    float playerRadius = 0.4f;
    float playerScale = 0.01f;

    // Camera
    float cameraFov = 60.0f;
    float cameraNear = 0.1f;
    float cameraFar = 500.0f;
    float followDistance = 2.2f;
    float followHeight = 1.2f;
    float shoulderOffset = 2.4f;
    float lookAhead = 5.0f;

    // Buildings
    int buildingGridSize = 100;
    float buildingWidth = 8.0f;
    float buildingDepth = 8.0f;
    float buildingMinHeight = 15.0f;
    float buildingMaxHeight = 40.0f;
    float streetWidth = 12.0f;
    float buildingRenderDistance = 150.0f;  // Max distance for building rendering
    int maxVisibleBuildings = 2000;         // Max buildings to render per frame
    float buildingTextureScale = 4.0f;

    // LOD
    float lodSwitchDistance = 210.0f;

    // Ground
    float groundSize = 500.0f;
    float groundTextureScale = 0.5f;

    // Snow effect (2D overlay)
    float snowDefaultSpeed = 7.0f;
    float snowDefaultAngle = 20.0f;
    float snowDefaultBlur = 3.0f;

    // Snow particles (3D billboards)
    int snowParticleCount = 2000;
    float snowSphereRadius = 50.0f;
    float snowParticleFallSpeed = 3.0f;
    float snowParticleSize = 0.1f;
    float snowWindStrength = 0.5f;

    // Cinematic
    float cinematicDuration = 3.0f;
    float cinematicMotionBlur = 2.5f;
    float introCharacterYaw = 225.0f;
    glm::vec3 introCharacterPos = glm::vec3(-120.0f, 0.1f, -120.0f);

    // FING Building position
    glm::vec3 fingBuildingPos = glm::vec3(80.0f, 10.0f, 80.0f);

    // Light direction
    glm::vec3 lightDir = glm::vec3(0.5f, 1.0f, 0.3f);

    // UI
    float introHeaderX = 730.0f;
    float introHeaderY = 80.0f;
    float introBodyLeftMargin = 45.0f;
    float introBodyStartY = 180.0f;
    float introLineHeight = 100.0f;
    float typewriterCharDelay = 0.04f;
    float typewriterLineDelay = 0.5f;

    // Debug
    bool showAxes = false;
    bool showShadowMap = false;
};

// Singleton loader for game configuration from XML
class ConfigLoader {
public:
    static GameSettings& get() {
        static GameSettings settings;
        return settings;
    }

    // Load configuration from XML file
    // Returns true on success, false on failure (uses defaults on failure)
    static bool load(const std::string& filename) {
        GameSettings& s = get();

        TiXmlDocument doc(filename);
        if (!doc.LoadFile()) {
            std::cerr << "ConfigLoader: Failed to load " << filename
                      << " - using defaults. Error: " << doc.ErrorDesc() << std::endl;
            return false;
        }

        TiXmlElement* root = doc.RootElement();
        if (!root || std::string(root->Value()) != "GameConfig") {
            std::cerr << "ConfigLoader: Invalid root element - using defaults" << std::endl;
            return false;
        }

        // Parse each section
        parseWindow(root->FirstChildElement("Window"), s);
        parseGraphics(root->FirstChildElement("Graphics"), s);
        parseFog(root->FirstChildElement("Fog"), s);
        parsePlayer(root->FirstChildElement("Player"), s);
        parseCamera(root->FirstChildElement("Camera"), s);
        parseBuildings(root->FirstChildElement("Buildings"), s);
        parseLOD(root->FirstChildElement("LOD"), s);
        parseGround(root->FirstChildElement("Ground"), s);
        parseSnow(root->FirstChildElement("Snow"), s);
        parseCinematic(root->FirstChildElement("Cinematic"), s);
        parseFingBuilding(root->FirstChildElement("FingBuilding"), s);
        parseLight(root->FirstChildElement("Light"), s);
        parseUI(root->FirstChildElement("UI"), s);
        parseDebug(root->FirstChildElement("Debug"), s);

        std::cout << "ConfigLoader: Loaded configuration from " << filename << std::endl;
        return true;
    }

private:
    // Helper to get float attribute
    static float getFloatAttr(TiXmlElement* elem, const char* name, float defaultVal) {
        if (!elem) return defaultVal;
        double val;
        if (elem->QueryDoubleAttribute(name, &val) == TIXML_SUCCESS) {
            return static_cast<float>(val);
        }
        return defaultVal;
    }

    // Helper to get int attribute
    static int getIntAttr(TiXmlElement* elem, const char* name, int defaultVal) {
        if (!elem) return defaultVal;
        int val;
        if (elem->QueryIntAttribute(name, &val) == TIXML_SUCCESS) {
            return val;
        }
        return defaultVal;
    }

    // Helper to get string attribute
    static std::string getStringAttr(TiXmlElement* elem, const char* name, const std::string& defaultVal) {
        if (!elem) return defaultVal;
        const char* val = elem->Attribute(name);
        return val ? std::string(val) : defaultVal;
    }

    // Helper to parse vec3 from comma-separated string "x, y, z"
    static glm::vec3 getVec3Attr(TiXmlElement* elem, const char* name, const glm::vec3& defaultVal) {
        if (!elem) return defaultVal;
        const char* val = elem->Attribute(name);
        if (!val) return defaultVal;

        glm::vec3 result = defaultVal;
        std::stringstream ss(val);
        char comma;
        if (ss >> result.x >> comma >> result.y >> comma >> result.z) {
            return result;
        }
        return defaultVal;
    }

    // Helper to get bool attribute ("true"/"false")
    static bool getBoolAttr(TiXmlElement* elem, const char* name, bool defaultVal) {
        if (!elem) return defaultVal;
        const char* val = elem->Attribute(name);
        if (!val) return defaultVal;
        std::string s(val);
        return s == "true" || s == "1" || s == "yes";
    }

    static void parseWindow(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.windowWidth = getIntAttr(elem, "width", s.windowWidth);
        s.windowHeight = getIntAttr(elem, "height", s.windowHeight);
        s.windowFullscreen = getBoolAttr(elem, "fullscreen", s.windowFullscreen);
        s.windowTitle = getStringAttr(elem, "title", s.windowTitle);
    }

    static void parseGraphics(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.shadowMapSize = getIntAttr(elem, "shadowMapSize", s.shadowMapSize);
        s.shadowOrthoSize = getFloatAttr(elem, "shadowOrthoSize", s.shadowOrthoSize);
        s.shadowNear = getFloatAttr(elem, "shadowNear", s.shadowNear);
        s.shadowFar = getFloatAttr(elem, "shadowFar", s.shadowFar);
        s.shadowDistance = getFloatAttr(elem, "shadowDistance", s.shadowDistance);
    }

    static void parseFog(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.fogDensity = getFloatAttr(elem, "density", s.fogDensity);
        s.fogDesaturation = getFloatAttr(elem, "desaturation", s.fogDesaturation);
        s.fogColor = getVec3Attr(elem, "color", s.fogColor);
    }

    static void parsePlayer(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.playerMoveSpeed = getFloatAttr(elem, "moveSpeed", s.playerMoveSpeed);
        s.playerTurnSpeed = getFloatAttr(elem, "turnSpeed", s.playerTurnSpeed);
        s.playerRadius = getFloatAttr(elem, "radius", s.playerRadius);
        s.playerScale = getFloatAttr(elem, "scale", s.playerScale);
    }

    static void parseCamera(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.cameraFov = getFloatAttr(elem, "fov", s.cameraFov);
        s.cameraNear = getFloatAttr(elem, "near", s.cameraNear);
        s.cameraFar = getFloatAttr(elem, "far", s.cameraFar);
        s.followDistance = getFloatAttr(elem, "followDistance", s.followDistance);
        s.followHeight = getFloatAttr(elem, "followHeight", s.followHeight);
        s.shoulderOffset = getFloatAttr(elem, "shoulderOffset", s.shoulderOffset);
        s.lookAhead = getFloatAttr(elem, "lookAhead", s.lookAhead);
    }

    static void parseBuildings(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.buildingGridSize = getIntAttr(elem, "gridSize", s.buildingGridSize);
        s.buildingWidth = getFloatAttr(elem, "width", s.buildingWidth);
        s.buildingDepth = getFloatAttr(elem, "depth", s.buildingDepth);
        s.buildingMinHeight = getFloatAttr(elem, "minHeight", s.buildingMinHeight);
        s.buildingMaxHeight = getFloatAttr(elem, "maxHeight", s.buildingMaxHeight);
        s.streetWidth = getFloatAttr(elem, "streetWidth", s.streetWidth);
        s.buildingRenderDistance = getFloatAttr(elem, "renderDistance", s.buildingRenderDistance);
        s.maxVisibleBuildings = getIntAttr(elem, "maxVisible", s.maxVisibleBuildings);
        s.buildingTextureScale = getFloatAttr(elem, "textureScale", s.buildingTextureScale);
    }

    static void parseLOD(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.lodSwitchDistance = getFloatAttr(elem, "switchDistance", s.lodSwitchDistance);
    }

    static void parseGround(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.groundSize = getFloatAttr(elem, "size", s.groundSize);
        s.groundTextureScale = getFloatAttr(elem, "textureScale", s.groundTextureScale);
    }

    static void parseSnow(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.snowDefaultSpeed = getFloatAttr(elem, "defaultSpeed", s.snowDefaultSpeed);
        s.snowDefaultAngle = getFloatAttr(elem, "defaultAngle", s.snowDefaultAngle);
        s.snowDefaultBlur = getFloatAttr(elem, "defaultBlur", s.snowDefaultBlur);
        s.snowParticleCount = getIntAttr(elem, "particleCount", s.snowParticleCount);
        s.snowSphereRadius = getFloatAttr(elem, "sphereRadius", s.snowSphereRadius);
        s.snowParticleFallSpeed = getFloatAttr(elem, "particleFallSpeed", s.snowParticleFallSpeed);
        s.snowParticleSize = getFloatAttr(elem, "particleSize", s.snowParticleSize);
        s.snowWindStrength = getFloatAttr(elem, "windStrength", s.snowWindStrength);
    }

    static void parseCinematic(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.cinematicDuration = getFloatAttr(elem, "duration", s.cinematicDuration);
        s.cinematicMotionBlur = getFloatAttr(elem, "motionBlur", s.cinematicMotionBlur);
        s.introCharacterYaw = getFloatAttr(elem, "introCharacterYaw", s.introCharacterYaw);
        s.introCharacterPos = getVec3Attr(elem, "introCharacterPos", s.introCharacterPos);
    }

    static void parseFingBuilding(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.fingBuildingPos = getVec3Attr(elem, "pos", s.fingBuildingPos);
    }

    static void parseLight(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.lightDir = getVec3Attr(elem, "dir", s.lightDir);
    }

    static void parseUI(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.introHeaderX = getFloatAttr(elem, "introHeaderX", s.introHeaderX);
        s.introHeaderY = getFloatAttr(elem, "introHeaderY", s.introHeaderY);
        s.introBodyLeftMargin = getFloatAttr(elem, "introBodyLeftMargin", s.introBodyLeftMargin);
        s.introBodyStartY = getFloatAttr(elem, "introBodyStartY", s.introBodyStartY);
        s.introLineHeight = getFloatAttr(elem, "introLineHeight", s.introLineHeight);
        s.typewriterCharDelay = getFloatAttr(elem, "typewriterCharDelay", s.typewriterCharDelay);
        s.typewriterLineDelay = getFloatAttr(elem, "typewriterLineDelay", s.typewriterLineDelay);
    }

    static void parseDebug(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.showAxes = getBoolAttr(elem, "showAxes", s.showAxes);
        s.showShadowMap = getBoolAttr(elem, "showShadowMap", s.showShadowMap);
    }
};

// Convenience macro for accessing config values
#define CONFIG ConfigLoader::get()
