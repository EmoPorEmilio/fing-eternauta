#pragma once
#include <glm/glm.hpp>

// All game configuration values in one place
// This makes it easy to tweak values and eventually load from a config file

namespace GameConfig {

// Window
constexpr int WINDOW_WIDTH = 1280;
constexpr int WINDOW_HEIGHT = 720;
constexpr const char* WINDOW_TITLE = "fing-eternauta";

// Graphics
constexpr int SHADOW_MAP_SIZE = 2048;
constexpr float SHADOW_ORTHO_SIZE = 100.0f;
constexpr float SHADOW_NEAR = 1.0f;
constexpr float SHADOW_FAR = 200.0f;
constexpr float SHADOW_DISTANCE = 80.0f;  // How far from player to position shadow camera

// Fog
constexpr float FOG_DENSITY = 0.02f;
constexpr float FOG_DESATURATION = 0.8f;
inline const glm::vec3 FOG_COLOR = glm::vec3(0.5f, 0.5f, 0.55f);

// Player
constexpr float PLAYER_MOVE_SPEED = 3.0f;
constexpr float PLAYER_TURN_SPEED = 10.0f;
constexpr float PLAYER_RADIUS = 0.4f;
constexpr float PLAYER_SCALE = 0.01f;

// Camera
constexpr float CAMERA_FOV = 60.0f;
constexpr float CAMERA_NEAR = 0.1f;
constexpr float CAMERA_FAR = 100.0f;
constexpr float FOLLOW_DISTANCE = 2.2f;
constexpr float FOLLOW_HEIGHT = 1.2f;
constexpr float SHOULDER_OFFSET = 2.4f;
constexpr float LOOK_AHEAD = 5.0f;

// Buildings
constexpr int BUILDING_GRID_SIZE = 100;
constexpr float BUILDING_WIDTH = 8.0f;
constexpr float BUILDING_DEPTH = 8.0f;
constexpr float BUILDING_MIN_HEIGHT = 15.0f;
constexpr float BUILDING_MAX_HEIGHT = 40.0f;
constexpr float STREET_WIDTH = 12.0f;
constexpr int BUILDING_RENDER_RADIUS = 3;  // Grid cells around player to render
constexpr float BUILDING_TEXTURE_SCALE = 4.0f;  // World units per texture repeat

// LOD
constexpr float LOD_SWITCH_DISTANCE = 70.0f;

// Ground
constexpr float GROUND_SIZE = 500.0f;
constexpr float GROUND_TEXTURE_SCALE = 0.5f;  // Tiles every 2 units

// Snow effect
constexpr float SNOW_DEFAULT_SPEED = 7.0f;
constexpr float SNOW_DEFAULT_ANGLE = 20.0f;
constexpr float SNOW_DEFAULT_BLUR = 3.0f;

// Cinematic
constexpr float CINEMATIC_DURATION = 3.0f;
constexpr float CINEMATIC_MOTION_BLUR = 0.85f;
constexpr float INTRO_CHARACTER_YAW = 225.0f;  // Facing FING building
inline const glm::vec3 INTRO_CHARACTER_POS = glm::vec3(0.0f, 0.1f, 0.0f);

// FING Building position
inline const glm::vec3 FING_BUILDING_POS = glm::vec3(80.0f, 10.0f, 80.0f);

// Light direction (normalized in shader)
inline const glm::vec3 LIGHT_DIR = glm::vec3(0.5f, 1.0f, 0.3f);

// UI - Intro text positioning
constexpr float INTRO_HEADER_X = 730.0f;
constexpr float INTRO_HEADER_Y = 80.0f;
constexpr float INTRO_BODY_LEFT_MARGIN = 45.0f;
constexpr float INTRO_BODY_START_Y = 180.0f;
constexpr float INTRO_LINE_HEIGHT = 100.0f;
constexpr float TYPEWRITER_CHAR_DELAY = 0.04f;
constexpr float TYPEWRITER_LINE_DELAY = 0.5f;

}  // namespace GameConfig
