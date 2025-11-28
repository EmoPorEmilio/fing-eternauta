#pragma once

#include <SDL.h>
#include "Renderer.h"
#include "Camera.h"
#include "Scene.h"
#include "LightManager.h"
#include "AssetManager.h"
#include "Constants.h"
#include "libraries/imgui/imgui.h"
#include "libraries/imgui/imgui_impl_sdl2.h"
#include "libraries/imgui/imgui_impl_opengl3.h"

class Application
{
public:
    Application();
    ~Application();

    bool initialize();
    void run();
    void cleanup();

    // Key debouncing structure - moved here so it can be used in function signatures
    struct KeyState
    {
        bool isPressed = false;
        float lastPressTime = 0.0f;
        static constexpr float DEBOUNCE_TIME = 0.2f; // 200ms minimum between presses
    };

private:
    Renderer renderer;
    Camera camera;
    Scene scene;
    LightManager lightManager;

    bool running;
    Uint64 prevTicks;
    double frequency;
    float elapsedSeconds = 0.0f;
    float prevFrameSeconds = 0.0f;
    float lastDeltaTime = 0.0f;

    void handleEvents();
    void update(float deltaTime);
    void render();

    // Key debouncing helper
    bool isKeyPressed(KeyState &keyState, bool currentlyPressed, float currentTime);

    // Simple alternative - just detect press/release transitions
    bool isKeyJustPressed(bool &wasPressed, bool currentlyPressed);

    // UI tab functions
    void renderMaterialTab();
    void renderModelsTab();
    void renderLightingTab();
    void renderSnowTab();
    void renderPerformanceTab();
    void renderSystemTab();

    // UI state
    bool uiOpen = false;
    bool cursorCaptured = true;
    bool imguiInitialized = false;
    // Tunables
    float uiAmbient = 0.2f;
    float uiSpecularStrength = 0.5f;
    float uiNormalStrength = 0.276f;
    float uiRoughnessBias = 0.0f;
    // FING transform UI
    float uiFingPos[3] = {0.0f, 119.900f, -222.300f};
    float uiFingScale = 21.3f;

    // Military model UI
    float uiMilitaryPos[3] = {0.0f, 0.0f, -100.0f};
    float uiMilitaryScale = 8.5f;
    bool uiMilitaryAnim = true;
    float uiMilitaryAnimSpeed = 1.0f;

    float uiWalkingPos[3] = {50.0f, 0.0f, -50.0f};
    float uiWalkingScale = 5.0f;
    bool uiWalkingAnim = true;
    float uiWalkingAnimSpeed = 1.0f;

    // Flashlight UI variables
    float uiFlashlightBrightness = 2.0f;
    float uiFlashlightCutoff = 25.0f;                // degrees
    float uiFlashlightColor[3] = {1.0f, 0.8f, 0.6f}; // warm white

    // Snow UI variables
    bool uiSnowEnabled = true;
    int uiSnowCount = 30000;
    float uiSnowFallSpeed = 10.0f;
    float uiSnowWindSpeed = 5.0f;
    float uiSnowWindDirection = 180.0f; // degrees
    float uiSnowSpriteSize = 0.05f;
    float uiSnowTimeScale = 1.0f;
    bool uiSnowBulletGround = true; // Bullet ground collision toggle

    // Snow performance UI
    bool uiSnowFrustumCulling = true;
    bool uiSnowLOD = true;
    int uiSnowMaxVisible = 100000;

    // Culling settings UI
    bool uiObjectCulling = true;
    bool uiObjectLOD = false;

    // Fog settings UI
    bool uiFogEnabled = true;
    float uiFogColor[3] = {0.0667f, 0.0784f, 0.0980f}; // R:17 G:20 B:25
    float uiFogDensity = 0.0050f;
    float uiFogDesaturationStrength = 0.79f;
    float uiFogAbsorptionDensity = 0.0427f; // how fast darkening grows with distance
    float uiFogAbsorptionStrength = 1.0f;   // how much darkening contributes

    // Camera UI
    float uiMoveSpeed = 30.0f; // 10x default (original 3.0)

    // Key debouncing instances
    KeyState keyL, keyC;

    // Simple alternative tracking
    bool wasLPressed = false;
    bool wasCPressed = false;

    // Overlay UI
    float uiOverlaySnowSpeed = 8.0f; // multiplier (1.0 = original)
    bool uiOverlayMotionBlur = true;
    float uiOverlayTrailPersistence = 5.55f; // higher = longer trails (decay per sec)
    float uiOverlayDirectionDeg = 162.0f;    // default fall direction
    float uiOverlayTrailGain = 3.0f;         // brighten accumulated result
    float uiOverlayAdvectionScale = 3.25f;   // UV/sec scale for advection
};
