#pragma once

#include <cmath>

/**
 * Application-wide constants to eliminate magic numbers
 * Organized by subsystem for clarity
 */

namespace Constants
{

    // Math constants
    namespace Math
    {
        constexpr float PI = 3.14159265359f;
        constexpr float TWO_PI = 2.0f * PI;
        constexpr float HALF_PI = PI * 0.5f;
        constexpr float DEG_TO_RAD = PI / 180.0f;
        constexpr float RAD_TO_DEG = 180.0f / PI;
        constexpr float EPSILON = 1e-6f;
    }

    // Rendering constants
    namespace Rendering
    {
        // Default window dimensions
        constexpr int DEFAULT_WINDOW_WIDTH = 960;
        constexpr int DEFAULT_WINDOW_HEIGHT = 540;

        // OpenGL version
        constexpr int OPENGL_VERSION_MAJOR = 4;
        constexpr int OPENGL_VERSION_MINOR = 5;

        // Depth buffer
        constexpr int DEPTH_BUFFER_BITS = 24;

        // Projection matrix
        constexpr float DEFAULT_FOV_DEGREES = 60.0f;
        constexpr float NEAR_PLANE = 0.1f;
        constexpr float FAR_PLANE = 2000.0f;

        // Culling distance for massive scenes
        constexpr float DEFAULT_CULL_DISTANCE = 400.0f;

        // Circle/sphere constants
        constexpr float CIRCLE_RADIUS = 1.0f;
        constexpr float SPHERE_RADIUS = 1.0f;
    }

    // Camera constants
    namespace Camera
    {
        constexpr float DEFAULT_X = 0.0f;
        constexpr float DEFAULT_Y = 1.6f; // Eye height
        constexpr float DEFAULT_Z = 3.0f;

        constexpr float DEFAULT_MOVE_SPEED = 3.0f;
        constexpr float FAST_MOVE_SPEED = 30.0f; // 10x default
        constexpr float MAX_MOVE_SPEED = 200.0f;

        constexpr float DEFAULT_MOUSE_SENSITIVITY = 0.1f;
        constexpr float DEFAULT_YAW = -90.0f; // Looking down -Z
        constexpr float DEFAULT_PITCH = 0.0f;

        constexpr float MAX_PITCH = 89.0f;
        constexpr float MIN_PITCH = -89.0f;
    }

    // Lighting constants
    namespace Lighting
    {
        // Ambient lighting levels
        constexpr float MINIMAL_AMBIENT = 0.01f;
        constexpr float LOW_AMBIENT = 0.03f;
        constexpr float DEFAULT_AMBIENT = 0.2f;
        constexpr float HIGH_AMBIENT = 0.5f;

        // Attenuation constants for point lights
        constexpr float CONSTANT_ATTENUATION = 1.0f;
        constexpr float LINEAR_ATTENUATION = 0.1f;
        constexpr float QUADRATIC_ATTENUATION = 0.01f;

        // Flashlight attenuation (stronger falloff)
        constexpr float FLASHLIGHT_LINEAR = 0.02f;
        constexpr float FLASHLIGHT_QUADRATIC = 0.002f;

        // Default light parameters
        constexpr float DEFAULT_LIGHT_HEIGHT = 10.0f;
        constexpr float DEFAULT_SPECULAR_POWER = 64.0f;
        constexpr float DEFAULT_SPECULAR_STRENGTH = 0.25f;

        // Flashlight defaults
        constexpr float DEFAULT_FLASHLIGHT_BRIGHTNESS = 2.0f;
        constexpr float DEFAULT_FLASHLIGHT_CUTOFF = 25.0f;     // degrees
        constexpr float FLASHLIGHT_OUTER_CUTOFF_FACTOR = 0.9f; // inner * 0.9
    }

    // Object management constants
    namespace Objects
    {
        // Performance presets
        constexpr int PRESET_MINIMAL = 3000;
        constexpr int PRESET_MEDIUM = 15000;
        constexpr int PRESET_MAXIMUM = 500000;

        // Spatial constraints
        constexpr float MIN_DISTANCE_BETWEEN_OBJECTS = 3.0f;
        constexpr float WORLD_SIZE = 500.0f;
        constexpr float DEFAULT_OBJECT_HEIGHT = 0.5f; // Half height of prism

        // LOD distances
        constexpr float HIGH_LOD_DISTANCE = 8.0f;
        constexpr float MEDIUM_LOD_DISTANCE = 25.0f;
        // Beyond MEDIUM_LOD_DISTANCE = LOW LOD

        // Object colors
        constexpr float DEFAULT_OBJECT_COLOR_R = 0.8f;
        constexpr float DEFAULT_OBJECT_COLOR_G = 0.8f;
        constexpr float DEFAULT_OBJECT_COLOR_B = 0.8f;
    }

    // Snow system constants
    namespace Snow
    {
        constexpr int DEFAULT_SNOW_COUNT = 30000;
        constexpr int MAX_SNOW_COUNT = 150000;

        constexpr float DEFAULT_FALL_SPEED = 10.0f;
        constexpr float DEFAULT_WIND_SPEED = 5.0f;
        constexpr float DEFAULT_WIND_DIRECTION = 180.0f; // degrees
        constexpr float DEFAULT_SPRITE_SIZE = 0.05f;
        constexpr float DEFAULT_TIME_SCALE = 1.0f;

        constexpr float DEFAULT_SPAWN_HEIGHT = 50.0f;
        constexpr float DEFAULT_SPAWN_RADIUS = 200.0f;
        constexpr float DEFAULT_FLOOR_Y = 0.0f;

        // Impact puffs
        constexpr float DEFAULT_SETTLE_DURATION = 0.35f; // seconds
        constexpr float DEFAULT_PUFF_LIFETIME = 0.45f;   // seconds
        constexpr float DEFAULT_PUFF_SIZE = 0.12f;       // world-space half-size
    }

    // Fog constants
    namespace Fog
    {
        // Default fog color (dark blue-gray)
        constexpr float DEFAULT_FOG_COLOR_R = 0.0667f; // R:17
        constexpr float DEFAULT_FOG_COLOR_G = 0.0784f; // G:20
        constexpr float DEFAULT_FOG_COLOR_B = 0.0980f; // B:25

        constexpr float DEFAULT_FOG_DENSITY = 0.0050f;
        constexpr float DEFAULT_DESATURATION_STRENGTH = 0.79f;
        constexpr float DEFAULT_ABSORPTION_DENSITY = 0.0427f;
        constexpr float DEFAULT_ABSORPTION_STRENGTH = 1.0f;

        // Fog calculation constants
        constexpr float LUMINANCE_R = 0.299f;
        constexpr float LUMINANCE_G = 0.587f;
        constexpr float LUMINANCE_B = 0.114f;
    }

    // Material constants
    namespace Materials
    {
        constexpr float DEFAULT_METALLIC = 0.0f;
        constexpr float DEFAULT_ROUGHNESS = 0.5f;
        constexpr float MIN_ROUGHNESS = 0.04f; // Prevent division by zero
        constexpr float MAX_ROUGHNESS = 1.0f;

        constexpr float DEFAULT_NORMAL_STRENGTH = 0.276f;
        constexpr float DEFAULT_SPECULAR_STRENGTH = 0.5f;
        constexpr float DEFAULT_ROUGHNESS_BIAS = 0.0f;
        constexpr float DEFAULT_AMBIENT = 0.5f;

        // PBR constants
        constexpr float DIELECTRIC_F0 = 0.04f; // Non-metallic F0
        constexpr float PBR_AMBIENT_FACTOR = 0.03f;
    }

    // Performance monitoring
    namespace Performance
    {
        constexpr int STATS_PRINT_INTERVAL_FRAMES = 60;
        constexpr float STATS_PRINT_INTERVAL_SECONDS = 1.0f;
        constexpr int PROGRESS_BAR_WIDTH = 50;
        constexpr int PROGRESS_UPDATE_INTERVAL_PERCENT = 5; // Show progress every 5%
    }

    // UI constants
    namespace UI
    {
        // Model transform defaults
        constexpr float DEFAULT_FING_POS_X = 0.0f;
        constexpr float DEFAULT_FING_POS_Y = 119.900f;
        constexpr float DEFAULT_FING_POS_Z = -222.300f;
        constexpr float DEFAULT_FING_SCALE = 21.3f;

        constexpr float DEFAULT_MILITARY_POS_X = 0.0f;
        constexpr float DEFAULT_MILITARY_POS_Y = 0.0f;
        constexpr float DEFAULT_MILITARY_POS_Z = -100.0f;
        constexpr float DEFAULT_MILITARY_SCALE = 8.5f;
        constexpr float DEFAULT_MILITARY_ANIM_SPEED = 1.0f;

        constexpr float DEFAULT_WALKING_POS_X = 50.0f;
        constexpr float DEFAULT_WALKING_POS_Y = 0.0f;
        constexpr float DEFAULT_WALKING_POS_Z = -50.0f;
        constexpr float DEFAULT_WALKING_SCALE = 5.0f;
        constexpr float DEFAULT_WALKING_ANIM_SPEED = 1.0f;

        // Overlay effects
        constexpr float DEFAULT_OVERLAY_SNOW_SPEED = 8.0f;
        constexpr float DEFAULT_OVERLAY_TRAIL_PERSISTENCE = 5.55f;
        constexpr float DEFAULT_OVERLAY_DIRECTION_DEG = 162.0f;
        constexpr float DEFAULT_OVERLAY_TRAIL_GAIN = 3.0f;
        constexpr float DEFAULT_OVERLAY_ADVECTION_SCALE = 3.25f;

        // Debug and logging
        constexpr int DEBUG_LOG_INTERVAL_FRAMES = 60; // Every 60 frames (1 sec at 60fps)
    }

    // Asset loading
    namespace Assets
    {
        // Maximum search depth for asset files
        constexpr int MAX_SEARCH_DEPTH = 4;

        // Texture parameters
        constexpr int TEXTURE_WRAP_MODE = 0x2901;  // GL_REPEAT
        constexpr int TEXTURE_MIN_FILTER = 0x2703; // GL_LINEAR_MIPMAP_LINEAR
        constexpr int TEXTURE_MAG_FILTER = 0x2601; // GL_LINEAR
        constexpr int TEXTURE_UNPACK_ALIGNMENT = 1;
    }
}
