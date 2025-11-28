#include "Application.h"
#include <iostream>
#include <cstdlib>

Application::Application() : running(false), prevTicks(0), frequency(0.0)
{
    // Initialize UI state
    uiOpen = false;
    cursorCaptured = true;
    uiAmbient = 0.2f;
    uiSpecularStrength = 0.5f;
    uiNormalStrength = 0.276f;
    uiRoughnessBias = 0.0f;
}

Application::~Application()
{
    cleanup();
}

bool Application::initialize()
{
    std::cout << "=== Application Initialization ===" << std::endl;

    std::cout << "Initializing renderer..." << std::endl;
    if (!renderer.initialize(960, 540))
    {
        std::cerr << "CRITICAL ERROR: Renderer initialization failed!" << std::endl;
        return false;
    }
    std::cout << "Renderer initialized successfully." << std::endl;

    std::cout << "Initializing scene..." << std::endl;
    if (!scene.initialize())
    {
        std::cerr << "CRITICAL ERROR: Scene initialization failed!" << std::endl;
        return false;
    }
    std::cout << "Scene initialized successfully." << std::endl;

    running = true;
    prevTicks = SDL_GetPerformanceCounter();
    frequency = (double)SDL_GetPerformanceFrequency();
    elapsedSeconds = 0.0f;
    prevFrameSeconds = 0.0f;

    // Enable relative mouse mode for FPS-style camera control
    SDL_SetRelativeMouseMode(SDL_TRUE);
    cursorCaptured = true;

    std::cout << "Initializing ImGui..." << std::endl;
    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();

    if (!ImGui_ImplSDL2_InitForOpenGL(renderer.window, renderer.getGLContext()))
    {
        std::cerr << "ERROR: ImGui SDL2 initialization failed!" << std::endl;
        return false;
    }

    if (!ImGui_ImplOpenGL3_Init("#version 450"))
    {
        std::cerr << "ERROR: ImGui OpenGL3 initialization failed!" << std::endl;
        return false;
    }

    imguiInitialized = true;
    std::cout << "ImGui initialized successfully." << std::endl;

    std::cout << "Initializing light manager GL resources..." << std::endl;
    // Initialize GL resources that require a valid context (e.g., flashlight UBO)
    lightManager.initializeGLResources();
    std::cout << "Light manager initialized successfully." << std::endl;

    // Verify OpenGL state after initialization
    if (!AssetManager::checkGLError("application initialization"))
    {
        std::cerr << "WARNING: OpenGL errors detected after initialization!" << std::endl;
    }

    std::cout << "=== Application Initialization Complete ===" << std::endl;
    return true;
}

void Application::run()
{
    while (running)
    {
        PerformanceProfiler::startFrame();

        handleEvents();

        Uint64 now = SDL_GetPerformanceCounter();
        float deltaTime = (float)((now - prevTicks) / frequency);
        prevTicks = now;
        lastDeltaTime = deltaTime;

        update(deltaTime);
        render();

        PerformanceProfiler::endFrame();
    }
}

void Application::cleanup()
{
    // Shutdown ImGui only if it was initialized
    if (imguiInitialized)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        imguiInitialized = false;
    }

    // Disable relative mouse mode
    SDL_SetRelativeMouseMode(SDL_FALSE);

    scene.cleanup();
    renderer.cleanup();
}

bool Application::isKeyPressed(KeyState &keyState, bool currentlyPressed, float currentTime)
{
    if (currentlyPressed && !keyState.isPressed)
    {
        // Key just pressed down
        if (currentTime - keyState.lastPressTime >= KeyState::DEBOUNCE_TIME)
        {
            keyState.isPressed = true;
            keyState.lastPressTime = currentTime;
            return true; // Trigger action
        }
    }
    else if (!currentlyPressed && keyState.isPressed)
    {
        // Key just released
        keyState.isPressed = false;
    }

    return false; // Don't trigger action
}

bool Application::isKeyJustPressed(bool &wasPressed, bool currentlyPressed)
{
    // Simple approach: detect transition from not-pressed to pressed
    if (currentlyPressed && !wasPressed)
    {
        wasPressed = true;
        return true; // Trigger action
    }
    else if (!currentlyPressed)
    {
        wasPressed = false;
    }

    return false; // Don't trigger action
}

void Application::handleEvents()
{
    SDL_Event e;
    while (SDL_PollEvent(&e))
    {
        // Always pass events to ImGui first (if initialized)
        if (imguiInitialized)
        {
            ImGui_ImplSDL2_ProcessEvent(&e);
        }

        // If ImGui wants to capture events, skip scene handling
        if (uiOpen && ImGui::GetIO().WantCaptureKeyboard)
        {
            // Only handle ESC to close menu when UI is capturing
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
            {
                uiOpen = false;
                cursorCaptured = true;
                SDL_SetRelativeMouseMode(SDL_TRUE);
            }
            continue;
        }

        if (e.type == SDL_QUIT)
            running = false;
        else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_ESCAPE)
        {
            uiOpen = !uiOpen;
            cursorCaptured = !uiOpen;
            SDL_SetRelativeMouseMode(cursorCaptured ? SDL_TRUE : SDL_FALSE);
        }
        else if (!uiOpen) // Only handle scene shortcuts when menu is closed
        {
            if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_SPACE)
            {
                lightManager.toggleFlashlight();
            }
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_TAB)
            {
                // Toggle relative mouse mode
                static bool mouseMode = true;
                mouseMode = !mouseMode;
                SDL_SetRelativeMouseMode(mouseMode ? SDL_TRUE : SDL_FALSE);
            }
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_1)
            {
                // Switch to minimal preset
                scene.setObjectCount(ObjectManager::PRESET_MINIMAL);
                std::cout << "Switched to MINIMAL preset: " << ObjectManager::PRESET_MINIMAL << " objects" << std::endl;
            }
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_2)
            {
                // Switch to medium preset
                scene.setObjectCount(ObjectManager::PRESET_MEDIUM);
                std::cout << "Switched to MEDIUM preset: " << ObjectManager::PRESET_MEDIUM << " objects" << std::endl;
            }
            else if (e.type == SDL_KEYDOWN && e.key.keysym.sym == SDLK_3)
            {
                // Switch to maximum preset
                scene.setObjectCount(ObjectManager::PRESET_MAXIMUM);
                std::cout << "Switched to MAXIMUM preset: " << ObjectManager::PRESET_MAXIMUM << " objects" << std::endl;
            }
            // Note: C and L key handling moved to update() function for proper debouncing
        }

        if (e.type == SDL_MOUSEMOTION && cursorCaptured)
        {
            // Handle mouse movement for camera look using relative movement
            float xoffset = static_cast<float>(e.motion.xrel);
            float yoffset = -static_cast<float>(e.motion.yrel); // Reversed since y-coordinates go from bottom to top
            camera.handleMouseInput(xoffset, yoffset);
        }
        else if (e.type == SDL_WINDOWEVENT &&
                 (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED ||
                  e.window.event == SDL_WINDOWEVENT_RESIZED))
        {
            int w = 0, h = 0;
            SDL_GL_GetDrawableSize(renderer.window, &w, &h);
            renderer.handleResize(w, h);
        }
    }
}

void Application::update(float deltaTime)
{
    PerformanceProfiler::startTimer("update");

    // Update camera only when menu is closed
    if (!uiOpen)
    {
        const Uint8 *keys = SDL_GetKeyboardState(nullptr);
        camera.setMovementSpeed(uiMoveSpeed);
        camera.handleInput(keys, deltaTime);
        camera.update(deltaTime);

        // Handle debounced key presses for toggle functions
        // C key - Toggle culling (using simple approach)
        bool cPressed = keys[SDL_SCANCODE_C] != 0;
        if (isKeyJustPressed(wasCPressed, cPressed))
        {
            scene.toggleCulling();
            uiObjectCulling = scene.isCullingEnabled(); // ✅ Sync UI with new state!
            std::cout << "Culling " << (uiObjectCulling ? "ENABLED" : "DISABLED") << " [KEYBOARD]" << std::endl;
        }

        // L key - Toggle LOD (using simple approach)
        bool lPressed = keys[SDL_SCANCODE_L] != 0;
        if (isKeyJustPressed(wasLPressed, lPressed))
        {
            scene.toggleLOD();
            uiObjectLOD = scene.isLODEnabled(); // ✅ Sync UI with new state!
            std::cout << "LOD " << (uiObjectLOD ? "ENABLED" : "DISABLED") << " [KEYBOARD]" << std::endl;
        }
    }

    // Always update scene so UI changes apply live
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = renderer.getProjection();
    scene.update(camera.getPosition(), deltaTime, view, projection);

    // Update performance counters
    PerformanceProfiler::setCounter("total_objects", scene.getObjectCount());

    // Keep flashlight synced to current camera even if menu is open
    lightManager.updateFlashlight(camera.getPosition(), camera.getFront());
    // Also update flashlight UBO data after position/direction change
    lightManager.updateFlashlightUBO();

    // Advance global elapsed time for overlays
    elapsedSeconds += deltaTime;

    PerformanceProfiler::endTimer("update");
}

void Application::render()
{
    PerformanceProfiler::startTimer("render");

    // Begin ImGui frame only if initialized
    if (imguiInitialized)
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
    }

    if (uiOpen)
    {
        ImGui::Begin("Advanced OpenGL Configuration", &uiOpen, ImGuiWindowFlags_AlwaysAutoResize);

        if (ImGui::BeginTabBar("ConfigTabs"))
        {
            if (ImGui::BeginTabItem("Materials"))
            {
                renderMaterialTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Models"))
            {
                renderModelsTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Lighting"))
            {
                renderLightingTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Snow & Effects"))
            {
                renderSnowTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("Performance"))
            {
                renderPerformanceTab();
                ImGui::EndTabItem();
            }

            if (ImGui::BeginTabItem("System"))
            {
                renderSystemTab();
                ImGui::EndTabItem();
            }

            ImGui::EndTabBar();
        }

        ImGui::Separator();
        ImGui::Text("Controls: ESC=Close, SPACE=Flashlight, TAB=Mouse, 1/2/3=Presets");
        ImGui::Text("Toggle Keys: C=Culling, L=LOD (keyboard + UI sync - now working!)");
        ImGui::End();
    }

    // Apply UI state to scene material
    scene.setAmbient(uiAmbient);
    scene.setSpecularStrength(uiSpecularStrength);
    scene.setNormalStrength(uiNormalStrength);
    scene.setRoughnessBias(uiRoughnessBias);
    scene.setFingPosition(glm::vec3(uiFingPos[0], uiFingPos[1], uiFingPos[2]));
    scene.setFingScale(uiFingScale);
    scene.setMilitaryPosition(glm::vec3(uiMilitaryPos[0], uiMilitaryPos[1], uiMilitaryPos[2]));
    scene.setMilitaryScale(uiMilitaryScale);
    scene.setMilitaryAnimEnabled(uiMilitaryAnim);
    scene.setMilitaryAnimSpeed(uiMilitaryAnimSpeed);

    scene.setWalkingPosition(glm::vec3(uiWalkingPos[0], uiWalkingPos[1], uiWalkingPos[2]));
    scene.setWalkingScale(uiWalkingScale);
    scene.setWalkingAnimEnabled(uiWalkingAnim);
    scene.setWalkingAnimSpeed(uiWalkingAnimSpeed);

    // Apply UI state to flashlight
    lightManager.setFlashlightBrightness(uiFlashlightBrightness);
    lightManager.setFlashlightColor(glm::vec3(uiFlashlightColor[0], uiFlashlightColor[1], uiFlashlightColor[2]));
    lightManager.setFlashlightCutoff(uiFlashlightCutoff);

    // Apply UI state to snow system
    scene.setSnowEnabled(uiSnowEnabled);
    scene.setSnowCount(uiSnowCount);
    scene.setSnowFallSpeed(uiSnowFallSpeed);
    scene.setSnowWindSpeed(uiSnowWindSpeed);
    scene.setSnowWindDirection(uiSnowWindDirection);
    scene.setSnowSpriteSize(uiSnowSpriteSize);
    scene.setSnowTimeScale(uiSnowTimeScale);
    scene.setSnowBulletGroundCollision(uiSnowBulletGround);

    // Apply performance settings
    scene.setSnowFrustumCulling(uiSnowFrustumCulling);
    scene.setSnowLOD(uiSnowLOD);
    scene.setSnowMaxVisible(uiSnowMaxVisible);

    // Apply culling settings
    scene.setObjectCulling(uiObjectCulling);
    scene.setObjectLOD(uiObjectLOD);

    // Apply fog settings
    scene.setFogEnabled(uiFogEnabled);
    scene.setFogColor(glm::vec3(uiFogColor[0], uiFogColor[1], uiFogColor[2]));
    scene.setFogDensity(uiFogDensity);
    scene.setFogDesaturationStrength(uiFogDesaturationStrength);
    scene.setFogAbsorption(uiFogAbsorptionDensity, uiFogAbsorptionStrength);

    // Debug flashlight state (gated by runtime env var OPENGL_ADV_DEBUG_LOGS)
    static bool gDebugLogs = (std::getenv("OPENGL_ADV_DEBUG_LOGS") != nullptr);
    static int debugCounter = 0;
    if (gDebugLogs)
    {
        if (debugCounter % 60 == 0)
        {
            std::cout << "=== FLASHLIGHT DEBUG ===" << std::endl;
            std::cout << "UI Brightness: " << uiFlashlightBrightness << std::endl;
            std::cout << "UI Cutoff: " << uiFlashlightCutoff << std::endl;
            std::cout << "LightManager Brightness: " << lightManager.getFlashlightBrightness() << std::endl;
            std::cout << "LightManager Cutoff: " << lightManager.getFlashlightCutoff() << std::endl;
            std::cout << "Flashlight On: " << (lightManager.isFlashlightOn() ? "YES" : "NO") << std::endl;
            std::cout << "Flashlight Pos: (" << lightManager.getFlashlightPosition().x << ", " << lightManager.getFlashlightPosition().y << ", " << lightManager.getFlashlightPosition().z << ")" << std::endl;
        }
        debugCounter++;
    }

    // Use overlay-enabled render path
    renderer.render(camera, scene, lightManager, elapsedSeconds, lastDeltaTime, uiOverlaySnowSpeed, uiOverlayMotionBlur, uiOverlayTrailPersistence, uiOverlayDirectionDeg, uiOverlayTrailGain, uiOverlayAdvectionScale);

    // Draw ImGui only if initialized
    if (imguiInitialized)
    {
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    }

    // Swap once everything (scene + UI) has been drawn
    SDL_GL_SwapWindow(renderer.window);

    PerformanceProfiler::endTimer("render");

    // Check for any OpenGL errors at the end of the frame
    AssetManager::checkGLError("end of frame");
}

void Application::renderMaterialTab()
{
    ImGui::Text("Surface Material Properties");
    ImGui::SliderFloat("Ambient", &uiAmbient, 0.0f, Constants::Materials::DEFAULT_AMBIENT, "%.3f");
    ImGui::SliderFloat("Specular Strength", &uiSpecularStrength, 0.0f, 1.0f, "%.3f");
    ImGui::SliderFloat("Normal Strength", &uiNormalStrength, 0.0f, 4.0f, "%.3f");
    ImGui::SliderFloat("Roughness Bias", &uiRoughnessBias, -0.3f, 0.3f, "%.3f");

    ImGui::Separator();
    ImGui::Text("Two-Stage Fog System - TRUE Object Disappearing!");
    ImGui::Checkbox("Fog Enabled", &uiFogEnabled);
    ImGui::ColorEdit3("Fog Color (atmospheric effect)", uiFogColor);
    ImGui::SliderFloat("Fog Density (disappearing speed)", &uiFogDensity, 0.0f, 1.0f, "%.4f");
    ImGui::SliderFloat("Fog Desaturation (global effect)", &uiFogDesaturationStrength, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Light Absorption Density", &uiFogAbsorptionDensity, 0.0f, 1.0f, "%.4f");
    ImGui::SliderFloat("Light Absorption Strength", &uiFogAbsorptionStrength, 0.0f, 1.0f, "%.2f");
    ImGui::Text("⚡ Objects blend to fog color, then to background for TRUE disappearing!");
}

void Application::renderModelsTab()
{
    ImGui::Text("FING Model Transform");
    ImGui::DragFloat3("Position##fing", uiFingPos, 0.1f);
    ImGui::DragFloat("Scale##fing", &uiFingScale, 0.1f, 0.1f, 100.0f);

    ImGui::Separator();
    ImGui::Text("MILITARY Model Transform");
    ImGui::DragFloat3("Position##military", uiMilitaryPos, 0.1f);
    ImGui::DragFloat("Scale##military", &uiMilitaryScale, 0.1f, 0.1f, 500.0f);
    ImGui::Checkbox("Idle Animation", &uiMilitaryAnim);
    ImGui::DragFloat("Animation Speed", &uiMilitaryAnimSpeed, 0.05f, 0.0f, 5.0f, "%.2f");

    ImGui::Separator();
    ImGui::Text("WALKING Model Transform");
    ImGui::DragFloat3("Position##walking", uiWalkingPos, 0.1f);
    ImGui::DragFloat("Scale##walking", &uiWalkingScale, 0.1f, 0.1f, 500.0f);
    ImGui::Checkbox("Walking Animation", &uiWalkingAnim);
    ImGui::DragFloat("Walking Speed", &uiWalkingAnimSpeed, 0.05f, 0.0f, 5.0f, "%.2f");

    ImGui::Separator();
    ImGui::Text("Camera Controls");
    ImGui::SliderFloat("Move Speed", &uiMoveSpeed, Constants::Camera::DEFAULT_MOVE_SPEED, Constants::Camera::MAX_MOVE_SPEED, "%.1f");
}

void Application::renderLightingTab()
{
    ImGui::Text("Flashlight Controls");
    ImGui::DragFloat("Brightness", &uiFlashlightBrightness, 0.1f, 0.1f, 10.0f);
    ImGui::DragFloat("Cutoff Angle", &uiFlashlightCutoff, 1.0f, 5.0f, 60.0f);
    ImGui::ColorEdit3("Color", uiFlashlightColor);

    if (ImGui::Button("Toggle Flashlight (SPACE)"))
    {
        lightManager.toggleFlashlight();
    }

    ImGui::Separator();
    ImGui::Text("Light Information");
    ImGui::Text("Flashlight: %s", lightManager.isFlashlightOn() ? "ON" : "OFF");
    auto pos = lightManager.getFlashlightPosition();
    ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
}

void Application::renderSnowTab()
{
    ImGui::Text("Snow System");
    ImGui::Checkbox("Enabled", &uiSnowEnabled);
    ImGui::SliderInt("Count", &uiSnowCount, 0, Constants::Snow::MAX_SNOW_COUNT);
    ImGui::DragFloat("Fall Speed", &uiSnowFallSpeed, 0.1f, 0.1f, 30.0f);
    ImGui::DragFloat("Wind Speed", &uiSnowWindSpeed, 0.1f, 0.0f, 20.0f);
    ImGui::DragFloat("Wind Direction", &uiSnowWindDirection, 1.0f, -180.0f, 180.0f);
    ImGui::DragFloat("Sprite Size", &uiSnowSpriteSize, 0.005f, 0.005f, 0.5f);
    ImGui::DragFloat("Time Scale", &uiSnowTimeScale, 0.1f, 0.1f, 5.0f);
    ImGui::Checkbox("Ground Collision", &uiSnowBulletGround);

    ImGui::Separator();
    ImGui::Text("Snow Performance");
    ImGui::Checkbox("Frustum Culling", &uiSnowFrustumCulling);
    ImGui::Checkbox("LOD System", &uiSnowLOD);
    ImGui::SliderInt("Max Visible", &uiSnowMaxVisible, 1000, 500000);

    ImGui::Separator();
    ImGui::Text("Overlay Effects");
    ImGui::DragFloat("Snow Speed", &uiOverlaySnowSpeed, 0.05f, 0.0f, 8.0f, "%.2f");
    ImGui::Checkbox("Motion Blur", &uiOverlayMotionBlur);
    ImGui::DragFloat("Trail Persistence", &uiOverlayTrailPersistence, 0.05f, 0.0f, 10.0f, "%.2f");
    ImGui::DragFloat("Direction (deg)", &uiOverlayDirectionDeg, 1.0f, 0.0f, 360.0f, "%.0f");
    ImGui::DragFloat("Trail Gain", &uiOverlayTrailGain, 0.05f, 0.1f, 3.0f, "%.2f");
    ImGui::DragFloat("Advection Scale", &uiOverlayAdvectionScale, 0.01f, 0.0f, 5.0f, "%.2f");

    if (ImGui::Button("Reset Overlay Trails"))
    {
        renderer.clearAccumulation();
    }

    ImGui::Separator();
}

void Application::renderPerformanceTab()
{
    ImGui::Text("Real-Time Performance Stats");
    const auto &stats = PerformanceProfiler::getCurrentFrame();
    float fps = (stats.frameTime > 0.0f) ? (1000.0f / stats.frameTime) : 0.0f;

    // Color-coded FPS display
    if (fps >= 60.0f)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f)); // Green
    else if (fps >= 30.0f)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f)); // Yellow
    else
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f)); // Red

    ImGui::Text("FPS: %.1f (%.2f ms)", fps, stats.frameTime);
    ImGui::PopStyleColor();

    ImGui::Text("Update: %.2f ms", stats.updateTime);
    ImGui::Text("Render: %.2f ms", stats.renderTime);
    ImGui::Text("Objects: %d/%d", stats.visibleObjects, stats.totalObjects);

    if (ImGui::Button("Print Detailed Stats to Console"))
    {
        PerformanceProfiler::printStats();
    }

    ImGui::Separator();
    ImGui::Text("Performance Presets");
    ImGui::Text("Current Objects: %d", scene.getObjectCount());

    if (ImGui::Button("Minimal (3K)") || ImGui::IsKeyPressed(ImGuiKey_1))
    {
        scene.setObjectCount(Constants::Objects::PRESET_MINIMAL);
    }
    ImGui::SameLine();
    if (ImGui::Button("Medium (15K)") || ImGui::IsKeyPressed(ImGuiKey_2))
    {
        scene.setObjectCount(Constants::Objects::PRESET_MEDIUM);
    }
    ImGui::SameLine();
    if (ImGui::Button("Maximum (500K)") || ImGui::IsKeyPressed(ImGuiKey_3))
    {
        scene.setObjectCount(Constants::Objects::PRESET_MAXIMUM);
    }
}

void Application::renderSystemTab()
{
    ImGui::Text("Culling & LOD System");
    ImGui::Checkbox("Object Culling", &uiObjectCulling);
    ImGui::Checkbox("Object LOD", &uiObjectLOD);
    ImGui::Text("Hotkeys: C=Toggle Culling, L=Toggle LOD");
    ImGui::Text("(Fixed: keyboard toggles now sync with UI checkboxes!)");

    ImGui::Separator();
    ImGui::Text("System Information");
    ImGui::Text("OpenGL: %s", glGetString(GL_VERSION));
    ImGui::Text("Renderer: %s", glGetString(GL_RENDERER));
    ImGui::Text("Vendor: %s", glGetString(GL_VENDOR));

    ImGui::Separator();
    ImGui::Text("Debug Options");
    ImGui::Text("OpenGL error checking: Enabled");
}
