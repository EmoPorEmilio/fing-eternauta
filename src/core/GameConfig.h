#pragma once
#include <glm/glm.hpp>
#include "ConfigLoader.h"

// All game configuration values in one place
// Values are loaded from config.xml at startup via ConfigLoader
// Access values using GameConfig::WINDOW_WIDTH, etc.

namespace GameConfig {

// Window
inline int& WINDOW_WIDTH = CONFIG.windowWidth;
inline int& WINDOW_HEIGHT = CONFIG.windowHeight;
inline std::string& WINDOW_TITLE = CONFIG.windowTitle;

// Graphics
inline int& SHADOW_MAP_SIZE = CONFIG.shadowMapSize;
inline float& SHADOW_ORTHO_SIZE = CONFIG.shadowOrthoSize;
inline float& SHADOW_NEAR = CONFIG.shadowNear;
inline float& SHADOW_FAR = CONFIG.shadowFar;
inline float& SHADOW_DISTANCE = CONFIG.shadowDistance;

// Fog
inline float& FOG_DENSITY = CONFIG.fogDensity;
inline float& FOG_DESATURATION = CONFIG.fogDesaturation;
inline glm::vec3& FOG_COLOR = CONFIG.fogColor;

// Player
inline float& PLAYER_MOVE_SPEED = CONFIG.playerMoveSpeed;
inline float& PLAYER_TURN_SPEED = CONFIG.playerTurnSpeed;
inline float& PLAYER_RADIUS = CONFIG.playerRadius;
inline float& PLAYER_SCALE = CONFIG.playerScale;

// Camera
inline float& CAMERA_FOV = CONFIG.cameraFov;
inline float& CAMERA_NEAR = CONFIG.cameraNear;
inline float& CAMERA_FAR = CONFIG.cameraFar;
inline float& FOLLOW_DISTANCE = CONFIG.followDistance;
inline float& FOLLOW_HEIGHT = CONFIG.followHeight;
inline float& SHOULDER_OFFSET = CONFIG.shoulderOffset;
inline float& LOOK_AHEAD = CONFIG.lookAhead;

// Buildings
inline int& BUILDING_GRID_SIZE = CONFIG.buildingGridSize;
inline float& BUILDING_WIDTH = CONFIG.buildingWidth;
inline float& BUILDING_DEPTH = CONFIG.buildingDepth;
inline float& BUILDING_MIN_HEIGHT = CONFIG.buildingMinHeight;
inline float& BUILDING_MAX_HEIGHT = CONFIG.buildingMaxHeight;
inline float& STREET_WIDTH = CONFIG.streetWidth;
inline float& BUILDING_RENDER_DISTANCE = CONFIG.buildingRenderDistance;
inline int& MAX_VISIBLE_BUILDINGS = CONFIG.maxVisibleBuildings;
inline float& BUILDING_TEXTURE_SCALE = CONFIG.buildingTextureScale;

// LOD
inline float& LOD_SWITCH_DISTANCE = CONFIG.lodSwitchDistance;

// Ground
inline float& GROUND_SIZE = CONFIG.groundSize;
inline float& GROUND_TEXTURE_SCALE = CONFIG.groundTextureScale;

// Snow effect (2D overlay)
inline float& SNOW_DEFAULT_SPEED = CONFIG.snowDefaultSpeed;
inline float& SNOW_DEFAULT_ANGLE = CONFIG.snowDefaultAngle;
inline float& SNOW_DEFAULT_BLUR = CONFIG.snowDefaultBlur;

// Snow particles (3D billboards)
inline int& SNOW_PARTICLE_COUNT = CONFIG.snowParticleCount;
inline float& SNOW_SPHERE_RADIUS = CONFIG.snowSphereRadius;
inline float& SNOW_PARTICLE_FALL_SPEED = CONFIG.snowParticleFallSpeed;
inline float& SNOW_PARTICLE_SIZE = CONFIG.snowParticleSize;
inline float& SNOW_WIND_STRENGTH = CONFIG.snowWindStrength;

// Cinematic
inline float& CINEMATIC_DURATION = CONFIG.cinematicDuration;
inline float& CINEMATIC_MOTION_BLUR = CONFIG.cinematicMotionBlur;
inline float& INTRO_CHARACTER_YAW = CONFIG.introCharacterYaw;
inline glm::vec3& INTRO_CHARACTER_POS = CONFIG.introCharacterPos;

// FING Building position
inline glm::vec3& FING_BUILDING_POS = CONFIG.fingBuildingPos;

// Light direction (normalized in shader)
inline glm::vec3& LIGHT_DIR = CONFIG.lightDir;

// UI - Intro text positioning
inline float& INTRO_HEADER_X = CONFIG.introHeaderX;
inline float& INTRO_HEADER_Y = CONFIG.introHeaderY;
inline float& INTRO_BODY_LEFT_MARGIN = CONFIG.introBodyLeftMargin;
inline float& INTRO_BODY_START_Y = CONFIG.introBodyStartY;
inline float& INTRO_LINE_HEIGHT = CONFIG.introLineHeight;
inline float& TYPEWRITER_CHAR_DELAY = CONFIG.typewriterCharDelay;
inline float& TYPEWRITER_LINE_DELAY = CONFIG.typewriterLineDelay;

// Debug
inline bool& SHOW_AXES = CONFIG.showAxes;
inline bool& SHOW_SHADOW_MAP = CONFIG.showShadowMap;

}  // namespace GameConfig
