#pragma once

#include <cstdint>

enum class ShaderType : uint8_t
{
    Phong = 0,
    Basic = 1,
    SnowGlow = 2
};

enum class CadenceSelection : uint8_t
{
    One = 0,
    Two = 1,
    Three = 2,
    Cycle = 3
};

struct CadencePreset
{
    int pyramids = 1000;
    float rotationScale = 1.0f;
    float fallSpeed = 0.0f; // units per second (downwards)
};

struct AppSettings
{
    int targetPyramidCount = 1000; // desired number to generate (capped by MAX_PYRAMIDS)
    ShaderType shaderType = ShaderType::SnowGlow;
    float fovDegrees = 80.0f;
    float ambientStrength = 0.3f;
    float diffuseStrength = 0.7f;
    float specularStrength = 0.3f;
    float shininess = 16.0f;
    bool frustumCullingEnabled = true;
    bool enableRotation = true;
    // Camera & input
    float cameraSpeed = 2.5f;
    float mouseSensitivity = 0.1f;
    // Projection
    float nearPlane = 0.1f;
    float farPlane = 600.0f;
    // Background color
    float bgR = 0.05f;
    float bgG = 0.10f;
    float bgB = 0.20f;
    // VSync
    bool vsyncEnabled = false;
    // Light position
    float lightPosX = 5.0f;
    float lightPosY = 8.0f;
    float lightPosZ = 5.0f;
    // Cadence system
    CadenceSelection cadenceSelection = CadenceSelection::Cycle;
    float cadenceCycleSeconds = 5.0f;
    CadencePreset cadence[3] = {
        {5000, 1.2f, 4.0f},  // Barely snowing (many small, noticeable)
        {12000, 1.5f, 7.0f}, // Snowing chill
        {20000, 2.0f, 10.0f} // Insane snow
    };
    // SnowGlow shader tweaks
    float snowGlowIntensity = 1.4f;     // glow multiplier
    float snowSparkleIntensity = 1.0f;  // sparkle contribution
    float snowSparkleThreshold = 0.85f; // sparkle threshold base
    float snowNoiseScale = 0.5f;        // noise UV scale
    float snowTintStrength = 0.10f;     // cool tint blend
    float snowFogStrength = 0.0f;       // distance fog to background
    float snowRimStrength = 0.4f;       // rim highlight intensity
    float snowRimPower = 3.0f;          // rim falloff power
    float snowExposure = 1.0f;          // tonemap exposure
    // Blizzard gust settings
    bool gustsEnabled = true;
    float gustIntervalSeconds = 12.0f;   // average interval between gusts
    float gustDurationSeconds = 3.0f;    // duration of a gust
    float gustFallMultiplier = 2.0f;     // multiply fall speed during gust
    float gustRotationMultiplier = 1.5f; // multiply rotation speed during gust
    // Motion blur settings
    bool motionBlurEnabled = true;
    float motionBlurTrail = 0.85f; // alpha for previous frame (0..0.98)
    // Snow material parameters
    float snowRoughness = 0.85f;
    float snowMetallic = 0.02f;
    float snowSSS = 0.4f;              // subsurface/translucency strength
    float snowAnisotropy = 0.6f;       // anisotropic highlight 0..1
    float snowBaseAlpha = 0.8f;        // overall transparency
    float snowEdgeFade = 2.5f;         // edge fade exponent
    float snowNormalAmplitude = 0.15f; // bump amount
    float snowCrackScale = 5.0f;       // scale for crack-like noise
    float snowCrackIntensity = 0.2f;   // contribution of crack facets

    // Debug/visibility controls
    bool debugOverlayEnabled = true;       // show on-screen debug overlay
    float impostorSpeedMultiplier = 20.0f; // global speed multiplier for impostors
    float impostorSizeMultiplier = 1.0f;   // global size multiplier for impostors
    float impostorMinWorldSize = 0.15f;    // clamp min half-size in world units
    float impostorMaxWorldSize = 3.0f;     // clamp max half-size in world units

    // Advanced culling optimization controls
    bool enableDistanceCulling = true;    // enable early distance-based culling
    bool enableScreenSpaceCulling = true; // enable screen-space pixel culling
    bool enableUniformBatching = true;    // enable uniform update batching
    float maxRenderDistance = 200.0f;     // maximum render distance for early culling
    float minScreenPixels = 0.5f;         // minimum screen pixels to render

    // LOD and performance controls
    float lodNearThreshold = 2.0f;    // pixels for near LOD
    float lodMidThreshold = 6.0f;     // pixels for mid LOD
    float screenCullMargin = 1.2f;    // margin for offscreen culling
    int maxImpostorsPerFrame = 30000; // safety cap for impostors
};
