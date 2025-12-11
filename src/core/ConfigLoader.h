#pragma once
#include <string>
#include <iostream>
#include <glm/glm.hpp>
#include "tinyxml.h"

// Runtime-configurable game settings loaded from config.xml
// Values are initialized with defaults matching the original GameConfig constants
struct GameSettings {
    // Window
    int windowWidth = 1280;
    int windowHeight = 720;
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
    int buildingRenderRadius = 3;
    float buildingTextureScale = 4.0f;

    // LOD
    float lodSwitchDistance = 210.0f;

    // Ground
    float groundSize = 500.0f;
    float groundTextureScale = 0.5f;

    // Snow effect
    float snowDefaultSpeed = 7.0f;
    float snowDefaultAngle = 20.0f;
    float snowDefaultBlur = 3.0f;

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

        std::cout << "ConfigLoader: Loaded configuration from " << filename << std::endl;
        return true;
    }

private:
    // Helper to get float from element
    static float getFloat(TiXmlElement* parent, const char* name, float defaultVal) {
        if (!parent) return defaultVal;
        TiXmlElement* elem = parent->FirstChildElement(name);
        if (!elem || !elem->GetText()) return defaultVal;
        return static_cast<float>(atof(elem->GetText()));
    }

    // Helper to get int from element
    static int getInt(TiXmlElement* parent, const char* name, int defaultVal) {
        if (!parent) return defaultVal;
        TiXmlElement* elem = parent->FirstChildElement(name);
        if (!elem || !elem->GetText()) return defaultVal;
        return atoi(elem->GetText());
    }

    // Helper to get string from element
    static std::string getString(TiXmlElement* parent, const char* name, const std::string& defaultVal) {
        if (!parent) return defaultVal;
        TiXmlElement* elem = parent->FirstChildElement(name);
        if (!elem || !elem->GetText()) return defaultVal;
        return std::string(elem->GetText());
    }

    static void parseWindow(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.windowWidth = getInt(elem, "width", s.windowWidth);
        s.windowHeight = getInt(elem, "height", s.windowHeight);
        s.windowTitle = getString(elem, "title", s.windowTitle);
    }

    static void parseGraphics(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.shadowMapSize = getInt(elem, "shadowMapSize", s.shadowMapSize);
        s.shadowOrthoSize = getFloat(elem, "shadowOrthoSize", s.shadowOrthoSize);
        s.shadowNear = getFloat(elem, "shadowNear", s.shadowNear);
        s.shadowFar = getFloat(elem, "shadowFar", s.shadowFar);
        s.shadowDistance = getFloat(elem, "shadowDistance", s.shadowDistance);
    }

    static void parseFog(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.fogDensity = getFloat(elem, "density", s.fogDensity);
        s.fogDesaturation = getFloat(elem, "desaturation", s.fogDesaturation);
        s.fogColor.r = getFloat(elem, "colorR", s.fogColor.r);
        s.fogColor.g = getFloat(elem, "colorG", s.fogColor.g);
        s.fogColor.b = getFloat(elem, "colorB", s.fogColor.b);
    }

    static void parsePlayer(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.playerMoveSpeed = getFloat(elem, "moveSpeed", s.playerMoveSpeed);
        s.playerTurnSpeed = getFloat(elem, "turnSpeed", s.playerTurnSpeed);
        s.playerRadius = getFloat(elem, "radius", s.playerRadius);
        s.playerScale = getFloat(elem, "scale", s.playerScale);
    }

    static void parseCamera(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.cameraFov = getFloat(elem, "fov", s.cameraFov);
        s.cameraNear = getFloat(elem, "near", s.cameraNear);
        s.cameraFar = getFloat(elem, "far", s.cameraFar);
        s.followDistance = getFloat(elem, "followDistance", s.followDistance);
        s.followHeight = getFloat(elem, "followHeight", s.followHeight);
        s.shoulderOffset = getFloat(elem, "shoulderOffset", s.shoulderOffset);
        s.lookAhead = getFloat(elem, "lookAhead", s.lookAhead);
    }

    static void parseBuildings(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.buildingGridSize = getInt(elem, "gridSize", s.buildingGridSize);
        s.buildingWidth = getFloat(elem, "width", s.buildingWidth);
        s.buildingDepth = getFloat(elem, "depth", s.buildingDepth);
        s.buildingMinHeight = getFloat(elem, "minHeight", s.buildingMinHeight);
        s.buildingMaxHeight = getFloat(elem, "maxHeight", s.buildingMaxHeight);
        s.streetWidth = getFloat(elem, "streetWidth", s.streetWidth);
        s.buildingRenderRadius = getInt(elem, "renderRadius", s.buildingRenderRadius);
        s.buildingTextureScale = getFloat(elem, "textureScale", s.buildingTextureScale);
    }

    static void parseLOD(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.lodSwitchDistance = getFloat(elem, "switchDistance", s.lodSwitchDistance);
    }

    static void parseGround(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.groundSize = getFloat(elem, "size", s.groundSize);
        s.groundTextureScale = getFloat(elem, "textureScale", s.groundTextureScale);
    }

    static void parseSnow(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.snowDefaultSpeed = getFloat(elem, "defaultSpeed", s.snowDefaultSpeed);
        s.snowDefaultAngle = getFloat(elem, "defaultAngle", s.snowDefaultAngle);
        s.snowDefaultBlur = getFloat(elem, "defaultBlur", s.snowDefaultBlur);
    }

    static void parseCinematic(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.cinematicDuration = getFloat(elem, "duration", s.cinematicDuration);
        s.cinematicMotionBlur = getFloat(elem, "motionBlur", s.cinematicMotionBlur);
        s.introCharacterYaw = getFloat(elem, "introCharacterYaw", s.introCharacterYaw);
        s.introCharacterPos.x = getFloat(elem, "introCharacterPosX", s.introCharacterPos.x);
        s.introCharacterPos.y = getFloat(elem, "introCharacterPosY", s.introCharacterPos.y);
        s.introCharacterPos.z = getFloat(elem, "introCharacterPosZ", s.introCharacterPos.z);
    }

    static void parseFingBuilding(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.fingBuildingPos.x = getFloat(elem, "posX", s.fingBuildingPos.x);
        s.fingBuildingPos.y = getFloat(elem, "posY", s.fingBuildingPos.y);
        s.fingBuildingPos.z = getFloat(elem, "posZ", s.fingBuildingPos.z);
    }

    static void parseLight(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.lightDir.x = getFloat(elem, "dirX", s.lightDir.x);
        s.lightDir.y = getFloat(elem, "dirY", s.lightDir.y);
        s.lightDir.z = getFloat(elem, "dirZ", s.lightDir.z);
    }

    static void parseUI(TiXmlElement* elem, GameSettings& s) {
        if (!elem) return;
        s.introHeaderX = getFloat(elem, "introHeaderX", s.introHeaderX);
        s.introHeaderY = getFloat(elem, "introHeaderY", s.introHeaderY);
        s.introBodyLeftMargin = getFloat(elem, "introBodyLeftMargin", s.introBodyLeftMargin);
        s.introBodyStartY = getFloat(elem, "introBodyStartY", s.introBodyStartY);
        s.introLineHeight = getFloat(elem, "introLineHeight", s.introLineHeight);
        s.typewriterCharDelay = getFloat(elem, "typewriterCharDelay", s.typewriterCharDelay);
        s.typewriterLineDelay = getFloat(elem, "typewriterLineDelay", s.typewriterLineDelay);
    }
};

// Convenience macro for accessing config values
#define CONFIG ConfigLoader::get()
