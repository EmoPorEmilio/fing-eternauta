#pragma once

#include <cstdint>

enum class ShaderType : uint8_t
{
    Phong = 0,
    Basic = 1,
    SnowGlow = 2,
    FrostCrystal = 3,
    Mix = 4
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
    float bgR = 0.08f;
    float bgG = 0.10f;
    float bgB = 0.14f;
    // VSync
    bool vsyncEnabled = false;
    // Light position
    float lightPosX = 5.0f;
    float lightPosY = 8.0f;
    float lightPosZ = 5.0f;
    // Cadence system
    CadenceSelection cadenceSelection = CadenceSelection::Cycle;
    float cadenceCycleSeconds = 10.0f;
    CadencePreset cadence[3] = {
        {5000, 1.2f, 4.0f},  // Barely snowing (many small, noticeable)
        {12000, 1.5f, 7.0f}, // Snowing chill
        {20000, 2.0f, 10.0f} // Insane snow
    };
    // SnowGlow shader tweaks
    float snowGlowIntensity = 0.9f;     // softer glow
    float snowSparkleIntensity = 0.35f; // subtle sparkle
    float snowSparkleThreshold = 0.92f; // fewer sparkles
    float snowNoiseScale = 1.2f;        // finer microstructure
    float snowTintStrength = 0.06f;     // gentle cool tint
    float snowFogStrength = 0.22f;      // more distance fog
    float snowRimStrength = 0.6f;       // restrained rim
    float snowRimPower = 2.0f;          // softer falloff
    float snowExposure = 1.0f;          // neutral exposure
    float snowMixAmount = 0.10f;        // slight lambert mix
    // Accumulation and jitter defaults
    float accumulationStrength = 0.45f;   // 0..1
    float accumulationCoverage = 0.55f;   // 0..1
    float accumulationNoiseScale = 0.08f; // tiling
    float accumulationColorR = 0.88f;
    float accumulationColorG = 0.93f;
    float accumulationColorB = 1.00f;
    float sparkleJitterIntensity = 0.25f; // 0..1
    float sparkleJitterSpeed = 3.0f;      // Hz-like
    // Depth-based appearance controls
    float depthDesatStrength = 0.35f; // desaturate by distance
    float depthBlueStrength = 0.30f;  // shift to blue by distance
    float fogHeightStrength = 0.18f;  // extra fog by height
    // Blizzard gust settings
    bool gustsEnabled = true;
    float gustIntervalSeconds = 12.0f;   // average interval between gusts
    float gustDurationSeconds = 3.0f;    // duration of a gust
    float gustFallMultiplier = 2.0f;     // multiply fall speed during gust
    float gustRotationMultiplier = 1.5f; // multiply rotation speed during gust
    // Wind sway for billboards
    bool windEnabled = true;
    float windStrength = 0.25f; // 0..1 amplitude
    float windFrequency = 0.6f; // Hz-like speed
    float windDirX = 0.9f;      // normalized direction
    float windDirY = 0.0f;
    float windDirZ = 0.4f;
    // Motion blur settings
    bool motionBlurEnabled = true;
    float motionBlurTrail = 0.85f; // alpha for previous frame (0..0.98)
    // Snow material parameters
    float snowRoughness = 0.90f;
    float snowMetallic = 0.00f;
    float snowSSS = 0.50f;             // stronger forward scatter
    float snowAnisotropy = 0.50f;      // balanced highlight
    float snowBaseAlpha = 0.75f;       // softer translucency
    float snowEdgeFade = 3.0f;         // stronger edge fade
    float snowNormalAmplitude = 0.10f; // subtle bumps
    float snowCrackScale = 5.0f;       // for static surfaces
    float snowCrackIntensity = 0.15f;  // gentle facets

    // Debug/visibility controls
    bool debugOverlayEnabled = false;      // clean default view
    float impostorSpeedMultiplier = 10.0f; // slower drift
    float impostorSizeMultiplier = 0.8f;   // slightly smaller sprites
    float impostorMinWorldSize = 0.05f;    // smaller minimum
    float impostorMaxWorldSize = 0.5f;     // smaller maximum

    // Extra accumulation surfaces
    bool sidePlatformEnabled = true;
    bool shelfEnabled = true;
    bool crateEnabled = true;
    bool columnEnabled = true;
    float surfaceScale = 1.0f; // scales all extra surfaces

    // Surprise effect: Aurora light sweep
    bool auroraEnabled = true;
    float auroraSpeed = 0.4f;     // speed of hue cycling
    float auroraHueRange = 0.35f; // extent of color variation

    // Advanced culling optimization controls
    bool enableDistanceCulling = true;     // enable early distance-based culling
    bool enableScreenSpaceCulling = false; // default OFF to avoid over-cull
    bool enableUniformBatching = true;     // enable uniform update batching
    float maxRenderDistance = 200.0f;      // maximum render distance for early culling
    float minScreenPixels = 0.2f;          // minimum screen pixels to render

    // LOD and performance controls
    float lodNearThreshold = 1.0f;    // pixels for near LOD (more detail)
    float lodMidThreshold = 3.0f;     // pixels for mid LOD
    float screenCullMargin = 1.2f;    // margin for offscreen culling
    int maxImpostorsPerFrame = 30000; // safety cap for impostors
};
