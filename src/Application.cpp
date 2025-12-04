#include "Application.h"
#include "ECSWorld.h"
#include <iostream>
#include <cstdlib>
#include <string>

Application::Application() : running(false), prevTicks(0), frequency(0.0)
{
    // Initialize UI state - start in main menu mode
    uiOpen = false;
    cursorCaptured = false;  // Cursor visible for menu
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

    // Initialize InputManager
    std::cout << "Initializing input manager..." << std::endl;
    InputManager::instance().initialize(renderer.window);
    std::cout << "Input manager initialized successfully." << std::endl;

    // Initialize ECS world before any managers
    std::cout << "Initializing ECS world..." << std::endl;
    ECSWorld::initialize();
    std::cout << "ECS world initialized successfully." << std::endl;

    // Initialize both scenes
    std::cout << "Initializing EmptyScene..." << std::endl;
    if (!emptyScene.initialize())
    {
        std::cerr << "WARNING: EmptyScene initialization failed!" << std::endl;
    }
    std::cout << "EmptyScene initialized." << std::endl;

    std::cout << "Initializing DemoScene..." << std::endl;
    if (!demoScene.initialize())
    {
        std::cerr << "WARNING: DemoScene initialization failed!" << std::endl;
    }
    std::cout << "DemoScene initialized." << std::endl;

    // Set default active scene to DemoScene (index 1)
    switchScene(currentSceneIndex);
    std::cout << "Active scene set to: " << (currentSceneIndex == 0 ? "EmptyScene" : "DemoScene") << std::endl;

    running = true;
    prevTicks = SDL_GetPerformanceCounter();
    frequency = (double)SDL_GetPerformanceFrequency();
    elapsedSeconds = 0.0f;
    prevFrameSeconds = 0.0f;

    // Start with cursor visible for UI interaction
    SDL_SetRelativeMouseMode(SDL_FALSE);
    cursorCaptured = false;

    std::cout << "Initializing ImGui..." << std::endl;
    // ImGui setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    // Custom Proyecto Viviana theme - dark slate blue with pink accents
    ImGuiStyle& style = ImGui::GetStyle();
    ImVec4* colors = style.Colors;

    // Main colors from design screenshots
    ImVec4 bgDark = ImVec4(0.145f, 0.165f, 0.200f, 1.0f);       // #252A33 - darkest
    ImVec4 bgMid = ImVec4(0.180f, 0.208f, 0.251f, 1.0f);        // #2E3540 - panels
    ImVec4 bgLight = ImVec4(0.220f, 0.255f, 0.306f, 1.0f);      // #38414E - buttons
    ImVec4 accent = ImVec4(0.906f, 0.298f, 0.475f, 1.0f);       // #E74C79 - pink accent
    ImVec4 accentDark = ImVec4(0.706f, 0.198f, 0.375f, 1.0f);   // darker pink
    ImVec4 textLight = ImVec4(0.847f, 0.871f, 0.914f, 1.0f);    // #D8DEE9 - light text
    ImVec4 textDim = ImVec4(0.502f, 0.557f, 0.627f, 1.0f);      // #808EA0 - dim text

    colors[ImGuiCol_WindowBg] = bgDark;
    colors[ImGuiCol_ChildBg] = bgMid;
    colors[ImGuiCol_PopupBg] = bgMid;
    colors[ImGuiCol_Border] = ImVec4(0.3f, 0.35f, 0.42f, 0.5f);
    colors[ImGuiCol_BorderShadow] = ImVec4(0.0f, 0.0f, 0.0f, 0.0f);
    colors[ImGuiCol_FrameBg] = bgLight;
    colors[ImGuiCol_FrameBgHovered] = ImVec4(0.28f, 0.32f, 0.39f, 1.0f);
    colors[ImGuiCol_FrameBgActive] = ImVec4(0.32f, 0.37f, 0.45f, 1.0f);
    colors[ImGuiCol_TitleBg] = bgDark;
    colors[ImGuiCol_TitleBgActive] = bgMid;
    colors[ImGuiCol_TitleBgCollapsed] = bgDark;
    colors[ImGuiCol_MenuBarBg] = bgDark;
    colors[ImGuiCol_ScrollbarBg] = bgDark;
    colors[ImGuiCol_ScrollbarGrab] = bgLight;
    colors[ImGuiCol_ScrollbarGrabHovered] = accent;
    colors[ImGuiCol_ScrollbarGrabActive] = accent;
    colors[ImGuiCol_CheckMark] = accent;
    colors[ImGuiCol_SliderGrab] = accent;
    colors[ImGuiCol_SliderGrabActive] = accentDark;
    colors[ImGuiCol_Button] = bgLight;
    colors[ImGuiCol_ButtonHovered] = ImVec4(0.30f, 0.35f, 0.42f, 1.0f);
    colors[ImGuiCol_ButtonActive] = accent;
    colors[ImGuiCol_Header] = bgLight;
    colors[ImGuiCol_HeaderHovered] = ImVec4(0.30f, 0.35f, 0.42f, 1.0f);
    colors[ImGuiCol_HeaderActive] = accent;
    colors[ImGuiCol_Separator] = accent;
    colors[ImGuiCol_SeparatorHovered] = accent;
    colors[ImGuiCol_SeparatorActive] = accent;
    colors[ImGuiCol_Tab] = bgLight;
    colors[ImGuiCol_TabHovered] = accent;
    colors[ImGuiCol_TabActive] = accentDark;
    colors[ImGuiCol_TabUnfocused] = bgMid;
    colors[ImGuiCol_TabUnfocusedActive] = bgLight;
    colors[ImGuiCol_Text] = textLight;
    colors[ImGuiCol_TextDisabled] = textDim;

    // Rounded style
    style.WindowRounding = 8.0f;
    style.ChildRounding = 8.0f;
    style.FrameRounding = 12.0f;
    style.PopupRounding = 8.0f;
    style.ScrollbarRounding = 8.0f;
    style.GrabRounding = 8.0f;
    style.TabRounding = 8.0f;
    style.WindowPadding = ImVec2(12, 12);
    style.FramePadding = ImVec2(10, 6);
    style.ItemSpacing = ImVec2(10, 8);
    style.ScrollbarSize = 14.0f;

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

    // Subscribe to events
    subscribeToEvents();
    std::cout << "Event subscriptions initialized." << std::endl;

    // Initialize UIManager
    std::cout << "Initializing UI manager..." << std::endl;
    UIManager::instance().initialize();
    std::cout << "UI manager initialized successfully." << std::endl;

    std::cout << "Initializing light manager GL resources..." << std::endl;
    // Initialize GL resources that require a valid context (e.g., flashlight UBO)
    lightManager.initializeGLResources();
    std::cout << "Light manager initialized successfully." << std::endl;

    // Verify OpenGL state after initialization
    if (!AssetManager::checkGLError("application initialization"))
    {
        std::cerr << "WARNING: OpenGL errors detected after initialization!" << std::endl;
    }

    // Set initial window title
    updateWindowTitle();

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
    // Unsubscribe from events first
    unsubscribeFromEvents();

    // Shutdown UIManager
    UIManager::instance().shutdown();

    // Shutdown ImGui only if it was initialized
    if (imguiInitialized)
    {
        ImGui_ImplOpenGL3_Shutdown();
        ImGui_ImplSDL2_Shutdown();
        ImGui::DestroyContext();
        imguiInitialized = false;
    }

    emptyScene.cleanup();
    demoScene.cleanup();

    // Shutdown ECS world after all managers have cleaned up
    ECSWorld::shutdown();

    // Shutdown InputManager
    InputManager::instance().shutdown();

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
    auto& inputMgr = InputManager::instance();

    // Update ImGui want flags
    if (imguiInitialized) {
        ImGuiIO& io = ImGui::GetIO();
        inputMgr.setImGuiWantsKeyboard(io.WantCaptureKeyboard);
        inputMgr.setImGuiWantsMouse(io.WantCaptureMouse);
    }

    // Process all SDL events through InputManager
    // InputManager publishes events, and we handle them via subscriptions
    if (!inputMgr.processEvents()) {
        running = false;
    }
}

void Application::update(float deltaTime)
{
    PerformanceProfiler::startTimer("update");

    // Allow camera movement with WASD when ImGui doesn't want keyboard
    auto& inputMgr = InputManager::instance();
    if (!inputMgr.doesImGuiWantKeyboard())
    {
        const Uint8 *keys = inputMgr.getKeyboardState();
        camera.setMovementSpeed(UIManager::instance().getMoveSpeed());
        camera.handleInput(keys, deltaTime);
    }
    camera.update(deltaTime);

    // Always update scene so UI changes apply live
    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 projection = renderer.getProjection();
    if (activeScene) {
        activeScene->update(camera.getPosition(), deltaTime, view, projection);
    }

    // Update performance counters (EmptyScene has no objects)
    PerformanceProfiler::setCounter("total_objects", 0);

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

    // Sync UI state from UIManager (ESC handling is in UIManager)
    auto& uiMgr = UIManager::instance();
    uiOpen = uiMgr.isOpen();

    // Begin ImGui frame only if initialized
    if (imguiInitialized)
    {
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplSDL2_NewFrame();
        ImGui::NewFrame();
    }

    // Handle different render modes
    if (m_renderMode == RenderMode::MAIN_MENU)
    {
        // Main menu mode - show scene selection screen
        if (imguiInitialized)
        {
            renderMainMenu();
        }

        // In main menu, just clear and show UI - no scene rendering
        glClearColor(0.08f, 0.09f, 0.11f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Draw ImGui
        if (imguiInitialized)
        {
            ImGui::Render();
            ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
        }
        SDL_GL_SwapWindow(renderer.window);
        PerformanceProfiler::endTimer("render");
        return;  // Skip scene rendering
    }
    else if (m_renderMode == RenderMode::WORLD)
    {
        // Minimal HUD in World mode - just show FPS and mode toggle hint
        if (imguiInitialized)
        {
            ImGuiIO& io = ImGui::GetIO();
            float windowWidth = io.DisplaySize.x;

            // Small overlay in corner
            ImGui::SetNextWindowPos(ImVec2(windowWidth - 200, 10));
            ImGui::SetNextWindowSize(ImVec2(190, 45));
            ImGui::SetNextWindowBgAlpha(0.5f);
            ImGui::Begin("##WorldModeHUD", nullptr,
                ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar |
                ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoDecoration);

            const auto& stats = PerformanceProfiler::getCurrentFrame();
            float fps = (stats.frameTime > 0.0f) ? (1000.0f / stats.frameTime) : 0.0f;
            ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "FPS: %.0f", fps);
            ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Tab: Editor Mode");
            ImGui::End();
        }
    }
    else if (imguiInitialized)
    {
        ImGuiIO& io = ImGui::GetIO();
        float windowWidth = io.DisplaySize.x;
        float windowHeight = io.DisplaySize.y;
        float menuBarHeight = 35.0f;

        // === TOP MENU BAR ===
        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(windowWidth, menuBarHeight));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 8));
        ImGui::Begin("##MenuBar", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

        ImGui::Text("PROYECTO VIVIANA");
        ImGui::SameLine(150);
        if (ImGui::Button("File")) {}
        ImGui::SameLine();
        if (ImGui::Button("Edit")) {}
        ImGui::SameLine();
        if (ImGui::Button("View")) {}
        ImGui::SameLine();
        if (ImGui::Button("Help")) {}

        // Right-aligned status
        ImGui::SameLine(windowWidth - 250);
        const auto& stats = PerformanceProfiler::getCurrentFrame();
        float fps = (stats.frameTime > 0.0f) ? (1000.0f / stats.frameTime) : 0.0f;
        ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "FPS: %.0f", fps);
        ImGui::SameLine();
        ImGui::Text("|  %s", lightManager.isFlashlightOn() ? "Flashlight ON" : "Flashlight OFF");

        ImGui::End();
        ImGui::PopStyleVar();

        if (uiOpen)
        {
            // === RIGHT SIDEBAR - SCENE PANEL ===
            ImGui::SetNextWindowPos(ImVec2(windowWidth - sidebarWidth, menuBarHeight));
            ImGui::SetNextWindowSize(ImVec2(sidebarWidth, windowHeight - menuBarHeight));
            ImGuiWindowFlags sidebarFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                            ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

            ImGui::Begin("##Sidebar", nullptr, sidebarFlags);

            // SCENE Header with border
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 headerStart = ImGui::GetCursorScreenPos();
            float headerWidth = ImGui::GetContentRegionAvail().x;

            // Draw header background
            drawList->AddRectFilled(
                ImVec2(headerStart.x - 12, headerStart.y - 5),
                ImVec2(headerStart.x + headerWidth + 12, headerStart.y + 35),
                IM_COL32(46, 53, 64, 255), 8.0f);

            ImGui::Dummy(ImVec2(0, 5));
            ImGui::SetCursorPosX((headerWidth - ImGui::CalcTextSize("SCENE").x) * 0.5f);
            ImGui::TextColored(ImVec4(0.847f, 0.871f, 0.914f, 1.0f), "SCENE");
            ImGui::Dummy(ImVec2(0, 10));

            ImGui::Spacing();

            // Category buttons in 2x2 grid layout (matching design)
            float btnWidth = (headerWidth - 15) * 0.5f;
            float btnHeight = 42.0f;

            // Helper lambda for styled buttons
            auto styledButton = [&](const char* label, Panel panel, float width) -> bool {
                bool isActive = (activePanel == panel);
                if (isActive) {
                    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.906f, 0.298f, 0.475f, 0.3f));
                    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.906f, 0.298f, 0.475f, 1.0f));
                    ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
                }
                bool clicked = ImGui::Button(label, ImVec2(width, btnHeight));
                if (isActive) {
                    ImGui::PopStyleVar();
                    ImGui::PopStyleColor(2);
                }
                return clicked;
            };

            // Row 1: CAMERA | OBJECTS
            if (styledButton("CAMERA", Panel::Camera, btnWidth))
                activePanel = (activePanel == Panel::Camera) ? Panel::None : Panel::Camera;
            ImGui::SameLine();
            if (styledButton("OBJECTS", Panel::Objects, btnWidth))
                activePanel = (activePanel == Panel::Objects) ? Panel::None : Panel::Objects;

            ImGui::Spacing();

            // Row 2: MATERIALS | LIGHTS
            if (styledButton("MATERIALS", Panel::Materials, btnWidth))
                activePanel = (activePanel == Panel::Materials) ? Panel::None : Panel::Materials;
            ImGui::SameLine();
            if (styledButton("LIGHTS", Panel::Lights, btnWidth))
                activePanel = (activePanel == Panel::Lights) ? Panel::None : Panel::Lights;

            ImGui::Spacing();

            // Pink separator
            ImVec2 sepPos = ImGui::GetCursorScreenPos();
            drawList->AddRectFilled(
                ImVec2(sepPos.x, sepPos.y + 5),
                ImVec2(sepPos.x + headerWidth, sepPos.y + 7),
                IM_COL32(231, 76, 121, 200));
            ImGui::Dummy(ImVec2(0, 15));

            // Row 3: VIEWPORT | SYSTEM
            if (styledButton("VIEWPORT", Panel::Viewport, btnWidth))
                activePanel = (activePanel == Panel::Viewport) ? Panel::None : Panel::Viewport;
            ImGui::SameLine();
            if (styledButton("SYSTEM", Panel::System, btnWidth))
                activePanel = (activePanel == Panel::System) ? Panel::None : Panel::System;

            ImGui::Spacing();
            ImGui::Spacing();

        // === ACTIVE PANEL CONTENT ===
        if (activePanel != Panel::None)
        {
            ImGui::Separator();
            ImGui::Spacing();

            // Panel header with pink accent line
            const char* panelName = "";
            switch (activePanel) {
                case Panel::Camera: panelName = "CAMERA"; break;
                case Panel::Objects: panelName = "OBJECTS"; break;
                case Panel::Materials: panelName = "MATERIALS"; break;
                case Panel::Lights: panelName = "LIGHTS"; break;
                case Panel::Viewport: panelName = "VIEWPORT"; break;
                case Panel::System: panelName = "SYSTEM"; break;
                default: break;
            }

            // Pink accent bar
            ImDrawList* drawList = ImGui::GetWindowDrawList();
            ImVec2 p = ImGui::GetCursorScreenPos();
            drawList->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + headerWidth, p.y + 3),
                                    IM_COL32(231, 76, 121, 255));
            ImGui::Dummy(ImVec2(0, 8));

            ImGui::Text("%s", panelName);
            ImGui::Spacing();

            // Panel content - delegate to UIManager for consistent UI
            switch (activePanel) {
                case Panel::Camera:
                    UIManager::instance().renderCameraPanel(camera);
                    break;
                case Panel::Objects:
                    UIManager::instance().renderObjectsPanel();
                    break;
                case Panel::Materials:
                    UIManager::instance().renderMaterialsPanel();
                    break;
                case Panel::Lights:
                    UIManager::instance().renderLightsPanel(lightManager);
                    break;
                case Panel::Viewport:
                    UIManager::instance().renderViewportPanel(renderer, camera.getPosition());
                    break;
                case Panel::System:
                    renderSystemTab();  // Keep scene switching in Application
                    UIManager::instance().renderPerformancePanel();
                    ImGui::Separator();
                    UIManager::instance().renderSnowPanel();
                    break;
                default: break;
            }
        }

        ImGui::End();  // Sidebar
        }  // end if (uiOpen)

        // === BOTTOM STATUS BAR (always visible) ===
        float statusHeight = 28.0f;
        float sidebarAdjust = uiOpen ? sidebarWidth : 0;
        ImGui::SetNextWindowPos(ImVec2(0, windowHeight - statusHeight));
        ImGui::SetNextWindowSize(ImVec2(windowWidth - sidebarAdjust, statusHeight));
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 5));
        ImGui::Begin("##StatusBar", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                     ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

        auto camPos = camera.getPosition();
        ImGui::TextColored(ImVec4(0.6f, 0.65f, 0.7f, 1.0f), "Pos: (%.1f, %.1f, %.1f)", camPos.x, camPos.y, camPos.z);
        ImGui::SameLine(200);
        ImGui::TextColored(ImVec4(0.5f, 0.55f, 0.6f, 1.0f), "ESC: Toggle Panel  |  SPACE: Flashlight  |  Right-click + drag: Look  |  WASD: Move");

        ImGui::End();  // StatusBar
        ImGui::PopStyleVar();
    }

    // Apply UI state to scene material (BaseScene has these)
    if (activeScene) {
        activeScene->setAmbient(uiAmbient);
        activeScene->setSpecularStrength(uiSpecularStrength);
        activeScene->setNormalStrength(uiNormalStrength);
        activeScene->setRoughnessBias(uiRoughnessBias);
    }

    // Apply UI state to flashlight
    lightManager.setFlashlightBrightness(uiFlashlightBrightness);
    lightManager.setFlashlightColor(glm::vec3(uiFlashlightColor[0], uiFlashlightColor[1], uiFlashlightColor[2]));
    lightManager.setFlashlightCutoff(uiFlashlightCutoff);

    // Apply fog settings (BaseScene has these)
    if (activeScene) {
        activeScene->setFogEnabled(uiFogEnabled);
        activeScene->setFogColor(glm::vec3(uiFogColor[0], uiFogColor[1], uiFogColor[2]));
        activeScene->setFogDensity(uiFogDensity);
        activeScene->setFogDesaturationStrength(uiFogDesaturationStrength);
        activeScene->setFogAbsorption(uiFogAbsorptionDensity, uiFogAbsorptionStrength);

        // Apply viewport settings (BaseScene debug visualization)
        activeScene->setDebugGridEnabled(uiShowGrid);
        activeScene->setDebugAxesEnabled(uiShowAxes);
        activeScene->setDebugGizmoEnabled(uiShowGizmo);
        activeScene->setDebugGridScale(uiGridScale);
        activeScene->setDebugGridFadeDistance(uiGridFadeDistance);
        activeScene->setViewportSize(renderer.getWidth(), renderer.getHeight());

        // Floor mode: 0=Grid, 1=Textured, 2=Both
        switch (uiFloorMode) {
            case 0: activeScene->setFloorMode(IScene::FloorMode::GridOnly); break;
            case 1: activeScene->setFloorMode(IScene::FloorMode::TexturedSnow); break;
            case 2: activeScene->setFloorMode(IScene::FloorMode::Both); break;
        }
    }

    // Note: Models, snow, objects, culling/LOD not available in EmptyScene

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

    // Render scene with or without snow overlay
    if (activeScene) {
        if (uiOverlayEnabled)
        {
            renderer.render(camera, *activeScene, lightManager, elapsedSeconds, lastDeltaTime, uiOverlaySnowSpeed, uiOverlayMotionBlur, uiOverlayTrailPersistence, uiOverlayDirectionDeg, uiOverlayTrailGain, uiOverlayAdvectionScale);
        }
        else
        {
            renderer.render(camera, *activeScene, lightManager);
        }
    }

    // Render escape menu if open (on top of everything)
    if (escapeMenuOpen && imguiInitialized)
    {
        renderEscapeMenu();
    }

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
    ImGui::Text("No models in EmptyScene");
    ImGui::Text("Use DemoScene for GLTF models");

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
    ImGui::Text("No snow system in EmptyScene");
    ImGui::Text("Use DemoScene for snow particles");

    ImGui::Separator();
    ImGui::Text("Overlay Effects");
    ImGui::Checkbox("Enable Snow Overlay", &uiOverlayEnabled);
    if (uiOverlayEnabled)
    {
        ImGui::Indent();
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
        ImGui::Unindent();
    }
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

    if (ImGui::Button("Print Detailed Stats to Console"))
    {
        PerformanceProfiler::printStats();
    }

    ImGui::Separator();
    ImGui::Text("EmptyScene - No objects loaded");
    ImGui::Text("Use DemoScene for object presets");
}

void Application::renderSystemTab()
{
    // Scene selection
    ImGui::Text("Active Scene");
    const char* sceneNames[] = { "EmptyScene", "DemoScene" };
    if (ImGui::Combo("Scene", &currentSceneIndex, sceneNames, IM_ARRAYSIZE(sceneNames))) {
        switchScene(currentSceneIndex);
    }

    if (currentSceneIndex == 0) {
        ImGui::TextColored(ImVec4(0.6f, 0.6f, 0.6f, 1.0f), "Clean 3D world with floor and fog");
    } else {
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Full demo: Snow, Models, Animation");
    }

    ImGui::Separator();
    ImGui::Text("System Information");
    ImGui::Text("OpenGL: %s", glGetString(GL_VERSION));
    ImGui::Text("Renderer: %s", glGetString(GL_RENDERER));
    ImGui::Text("Vendor: %s", glGetString(GL_VENDOR));

    ImGui::Separator();
    ImGui::Text("Debug Options");
    ImGui::Text("OpenGL error checking: Enabled");
}

void Application::renderViewportTab()
{
    ImGui::Text("Blender-Style Viewport Settings");

    ImGui::Separator();
    ImGui::Text("Floor Mode");
    const char* floorModes[] = { "Grid Only", "Textured Snow", "Both" };
    ImGui::Combo("Floor Style", &uiFloorMode, floorModes, IM_ARRAYSIZE(floorModes));

    ImGui::Separator();
    ImGui::Text("Debug Visualization");
    ImGui::Checkbox("Show Grid", &uiShowGrid);
    if (uiShowGrid)
    {
        ImGui::Indent();
        ImGui::SliderFloat("Grid Scale", &uiGridScale, 0.1f, 10.0f, "%.1f m");
        ImGui::SliderFloat("Fade Distance", &uiGridFadeDistance, 50.0f, 500.0f, "%.0f m");
        ImGui::Unindent();
    }

    ImGui::Checkbox("Show Origin Axes", &uiShowAxes);
    ImGui::Checkbox("Show Corner Gizmo", &uiShowGizmo);
    ImGui::Checkbox("Show Info Overlay", &uiShowInfoOverlay);

    ImGui::Separator();
    ImGui::Text("Viewport Info");
    ImGui::Text("Resolution: %d x %d", renderer.getWidth(), renderer.getHeight());
    auto camPos = camera.getPosition();
    ImGui::Text("Camera: (%.1f, %.1f, %.1f)", camPos.x, camPos.y, camPos.z);

    if (ImGui::Button("Reset Viewport Settings"))
    {
        uiShowGrid = true;
        uiShowAxes = true;
        uiShowGizmo = true;
        uiShowInfoOverlay = true;
        uiGridScale = 1.0f;
        uiGridFadeDistance = 150.0f;
        uiFloorMode = 0;
    }
}

void Application::renderMainMenu()
{
    ImGuiIO& io = ImGui::GetIO();
    float windowWidth = io.DisplaySize.x;
    float windowHeight = io.DisplaySize.y;

    // Center the menu panel
    float panelWidth = 400.0f;
    float panelHeight = 500.0f;
    float panelX = (windowWidth - panelWidth) / 2.0f;
    float panelY = (windowHeight - panelHeight) / 2.0f;

    ImGui::SetNextWindowPos(ImVec2(panelX, panelY));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

    ImGui::Begin("##MainMenu", nullptr, flags);

    // Title
    ImGui::Dummy(ImVec2(0, 15));
    float titleWidth = ImGui::CalcTextSize("PROYECTO VIVIANA").x;
    ImGui::SetCursorPosX((panelWidth - titleWidth) / 2.0f);
    ImGui::PushFont(nullptr);  // Use default font
    ImGui::TextColored(ImVec4(0.906f, 0.298f, 0.475f, 1.0f), "PROYECTO VIVIANA");
    ImGui::PopFont();

    ImGui::Dummy(ImVec2(0, 8));
    float subtitleWidth = ImGui::CalcTextSize("OpenGL 4.5 Graphics Engine").x;
    ImGui::SetCursorPosX((panelWidth - subtitleWidth) / 2.0f);
    ImGui::TextColored(ImVec4(0.6f, 0.65f, 0.7f, 1.0f), "OpenGL 4.5 Graphics Engine");

    ImGui::Dummy(ImVec2(0, 20));

    // Separator
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 15));

    // Mode selection text
    float selectWidth = ImGui::CalcTextSize("Select Mode").x;
    ImGui::SetCursorPosX((panelWidth - selectWidth) / 2.0f);
    ImGui::TextColored(ImVec4(0.847f, 0.871f, 0.914f, 1.0f), "Select Mode");

    ImGui::Dummy(ImVec2(0, 15));

    // Buttons - centered and styled
    float buttonWidth = 280.0f;
    float buttonHeight = 45.0f;
    float buttonX = (panelWidth - buttonWidth) / 2.0f;

    // Editor Mode button (opens EmptyScene with full UI)
    ImGui::SetCursorPosX(buttonX);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.220f, 0.255f, 0.306f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.906f, 0.298f, 0.475f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.906f, 0.298f, 0.475f, 1.0f));

    if (ImGui::Button("Editor", ImVec2(buttonWidth, buttonHeight)))
    {
        m_renderMode = RenderMode::EDITOR;
        switchScene(0);  // EmptyScene (calls updateWindowTitle)
        uiOpen = true;
        cursorCaptured = false;
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }
    ImGui::PopStyleColor(3);

    // Editor description
    ImGui::SetCursorPosX(buttonX + 10);
    ImGui::TextColored(ImVec4(0.5f, 0.55f, 0.6f, 1.0f), "Empty scene with full UI panels and controls");

    ImGui::Dummy(ImVec2(0, 12));

    // Demo Scene button (opens DemoScene with full content)
    ImGui::SetCursorPosX(buttonX);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.220f, 0.255f, 0.306f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.298f, 0.686f, 0.314f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.298f, 0.686f, 0.314f, 1.0f));

    if (ImGui::Button("Demo Scene", ImVec2(buttonWidth, buttonHeight)))
    {
        m_renderMode = RenderMode::EDITOR;
        switchScene(1);  // DemoScene (calls updateWindowTitle)
        uiOpen = true;
        cursorCaptured = false;
        SDL_SetRelativeMouseMode(SDL_FALSE);
    }
    ImGui::PopStyleColor(3);

    // Demo description
    ImGui::SetCursorPosX(buttonX + 10);
    ImGui::TextColored(ImVec4(0.5f, 0.55f, 0.6f, 1.0f), "Full demo: 500k objects, snow, GLTF models");

    ImGui::Dummy(ImVec2(0, 12));

    // Empty Scene button (opens EmptyScene in World mode)
    ImGui::SetCursorPosX(buttonX);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.220f, 0.255f, 0.306f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.6f, 0.8f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.6f, 0.8f, 1.0f));

    if (ImGui::Button("Empty Scene", ImVec2(buttonWidth, buttonHeight)))
    {
        m_renderMode = RenderMode::WORLD;
        switchScene(0);  // EmptyScene (calls updateWindowTitle)
        uiOpen = false;
        cursorCaptured = true;
        SDL_SetRelativeMouseMode(SDL_TRUE);
    }
    ImGui::PopStyleColor(3);

    // Empty scene description
    ImGui::SetCursorPosX(buttonX + 10);
    ImGui::TextColored(ImVec4(0.5f, 0.55f, 0.6f, 1.0f), "Clean world with snow cube, immersive view");

    ImGui::Dummy(ImVec2(0, 20));

    // Separator before hints
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 10));

    // Hint text
    float hintWidth = ImGui::CalcTextSize("Tab: Toggle Editor/World  |  ESC: Toggle Panel").x;
    ImGui::SetCursorPosX((panelWidth - hintWidth) / 2.0f);
    ImGui::TextColored(ImVec4(0.4f, 0.45f, 0.5f, 1.0f), "Tab: Toggle Editor/World  |  ESC: Toggle Panel");

    ImGui::End();
}

void Application::renderEscapeMenu()
{
    ImGuiIO& io = ImGui::GetIO();
    float windowWidth = io.DisplaySize.x;
    float windowHeight = io.DisplaySize.y;

    // Semi-transparent overlay background
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImVec2(windowWidth, windowHeight));
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.0f, 0.0f, 0.0f, 0.5f));
    ImGui::Begin("##EscapeOverlay", nullptr,
        ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoTitleBar |
        ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoInputs);
    ImGui::End();
    ImGui::PopStyleColor();

    // Center the menu panel
    float panelWidth = 350.0f;
    float panelHeight = 380.0f;
    float panelX = (windowWidth - panelWidth) / 2.0f;
    float panelY = (windowHeight - panelHeight) / 2.0f;

    ImGui::SetNextWindowPos(ImVec2(panelX, panelY));
    ImGui::SetNextWindowSize(ImVec2(panelWidth, panelHeight));

    ImGuiWindowFlags flags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                             ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

    ImGui::Begin("##EscapeMenu", nullptr, flags);

    // Title
    ImGui::Dummy(ImVec2(0, 10));
    float titleWidth = ImGui::CalcTextSize("PAUSED").x;
    ImGui::SetCursorPosX((panelWidth - titleWidth) / 2.0f);
    ImGui::TextColored(ImVec4(0.906f, 0.298f, 0.475f, 1.0f), "PAUSED");

    ImGui::Dummy(ImVec2(0, 5));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 15));

    // Buttons
    float buttonWidth = 250.0f;
    float buttonHeight = 40.0f;
    float buttonX = (panelWidth - buttonWidth) / 2.0f;

    // Resume button
    ImGui::SetCursorPosX(buttonX);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.220f, 0.255f, 0.306f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.298f, 0.686f, 0.314f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.298f, 0.686f, 0.314f, 1.0f));
    if (ImGui::Button("Resume", ImVec2(buttonWidth, buttonHeight)))
    {
        escapeMenuOpen = false;
        if (m_renderMode == RenderMode::WORLD) {
            SDL_SetRelativeMouseMode(SDL_TRUE);
            cursorCaptured = true;
        }
    }
    ImGui::PopStyleColor(3);

    ImGui::Dummy(ImVec2(0, 8));

    // Switch Scene section
    ImGui::SetCursorPosX(buttonX);
    ImGui::TextColored(ImVec4(0.6f, 0.65f, 0.7f, 1.0f), "Switch Scene:");
    ImGui::Dummy(ImVec2(0, 5));

    // Scene buttons side by side
    float halfBtnWidth = (buttonWidth - 10) / 2.0f;
    ImGui::SetCursorPosX(buttonX);

    bool isEmptyScene = (currentSceneIndex == 0);
    if (isEmptyScene) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.906f, 0.298f, 0.475f, 0.5f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.220f, 0.255f, 0.306f, 1.0f));
    }
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.906f, 0.298f, 0.475f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.906f, 0.298f, 0.475f, 1.0f));
    if (ImGui::Button("EmptyScene", ImVec2(halfBtnWidth, buttonHeight)))
    {
        m_renderMode = m_renderMode;  // Keep current mode
        switchScene(0);
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    bool isDemoScene = (currentSceneIndex == 1);
    if (isDemoScene) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.906f, 0.298f, 0.475f, 0.5f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.220f, 0.255f, 0.306f, 1.0f));
    }
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.906f, 0.298f, 0.475f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.906f, 0.298f, 0.475f, 1.0f));
    if (ImGui::Button("DemoScene", ImVec2(halfBtnWidth, buttonHeight)))
    {
        m_renderMode = m_renderMode;  // Keep current mode
        switchScene(1);
    }
    ImGui::PopStyleColor(3);

    ImGui::Dummy(ImVec2(0, 8));

    // Switch Mode section
    ImGui::SetCursorPosX(buttonX);
    ImGui::TextColored(ImVec4(0.6f, 0.65f, 0.7f, 1.0f), "Switch Mode:");
    ImGui::Dummy(ImVec2(0, 5));

    // Mode buttons side by side
    ImGui::SetCursorPosX(buttonX);

    bool isEditor = (m_renderMode == RenderMode::EDITOR);
    if (isEditor) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.6f, 0.8f, 0.5f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.220f, 0.255f, 0.306f, 1.0f));
    }
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.6f, 0.8f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.6f, 0.8f, 1.0f));
    if (ImGui::Button("Editor", ImVec2(halfBtnWidth, buttonHeight)))
    {
        m_renderMode = RenderMode::EDITOR;
        uiOpen = true;
        escapeMenuOpen = false;
        SDL_SetRelativeMouseMode(SDL_FALSE);
        cursorCaptured = false;
        updateWindowTitle();
    }
    ImGui::PopStyleColor(3);

    ImGui::SameLine();

    bool isWorld = (m_renderMode == RenderMode::WORLD);
    if (isWorld) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.4f, 0.6f, 0.8f, 0.5f));
    } else {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.220f, 0.255f, 0.306f, 1.0f));
    }
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.4f, 0.6f, 0.8f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.4f, 0.6f, 0.8f, 1.0f));
    if (ImGui::Button("World", ImVec2(halfBtnWidth, buttonHeight)))
    {
        m_renderMode = RenderMode::WORLD;
        uiOpen = false;
        escapeMenuOpen = false;
        SDL_SetRelativeMouseMode(SDL_TRUE);
        cursorCaptured = true;
        updateWindowTitle();
    }
    ImGui::PopStyleColor(3);

    ImGui::Dummy(ImVec2(0, 15));
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0, 10));

    // Main Menu button
    ImGui::SetCursorPosX(buttonX);
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.220f, 0.255f, 0.306f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.7f, 0.3f, 0.3f, 0.7f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.7f, 0.3f, 0.3f, 1.0f));
    if (ImGui::Button("Return to Main Menu", ImVec2(buttonWidth, buttonHeight)))
    {
        escapeMenuOpen = false;
        m_renderMode = RenderMode::MAIN_MENU;
        uiOpen = false;
        SDL_SetRelativeMouseMode(SDL_FALSE);
        cursorCaptured = false;
        updateWindowTitle();
    }
    ImGui::PopStyleColor(3);

    ImGui::Dummy(ImVec2(0, 10));

    // Hint
    float hintWidth = ImGui::CalcTextSize("Press ESC to close").x;
    ImGui::SetCursorPosX((panelWidth - hintWidth) / 2.0f);
    ImGui::TextColored(ImVec4(0.4f, 0.45f, 0.5f, 1.0f), "Press ESC to close");

    ImGui::End();
}

// Event handling implementation
void Application::subscribeToEvents()
{
    auto& bus = events::EventBus::instance();

    m_keyPressedSub = bus.subscribe<events::KeyPressedEvent>(
        [this](const events::KeyPressedEvent& e) { onKeyPressed(e); });

    m_windowResizedSub = bus.subscribe<events::WindowResizedEvent>(
        [this](const events::WindowResizedEvent& e) { onWindowResized(e); });

    m_windowClosedSub = bus.subscribe<events::WindowClosedEvent>(
        [this](const events::WindowClosedEvent& e) { onWindowClosed(e); });

    m_cameraLookSub = bus.subscribe<events::CameraLookRequestEvent>(
        [this](const events::CameraLookRequestEvent& e) { onCameraLookRequest(e); });

    // Set up ImGui event preprocessor
    InputManager::instance().setEventPreprocessor([this](const SDL_Event& event) {
        if (imguiInitialized) {
            ImGui_ImplSDL2_ProcessEvent(&event);
        }
    });

    std::cout << "[Application] Subscribed to events" << std::endl;
}

void Application::unsubscribeFromEvents()
{
    auto& bus = events::EventBus::instance();
    bus.unsubscribe(m_keyPressedSub);
    bus.unsubscribe(m_windowResizedSub);
    bus.unsubscribe(m_windowClosedSub);
    bus.unsubscribe(m_cameraLookSub);

    // Clear preprocessor
    InputManager::instance().setEventPreprocessor(nullptr);

    std::cout << "[Application] Unsubscribed from events" << std::endl;
}

void Application::onKeyPressed(const events::KeyPressedEvent& event)
{
    // SPACE/flashlight is handled by LightManager via FlashlightToggleEvent

    // ESC toggles escape menu (when not in main menu)
    if (event.key == events::KeyCode::Escape && !event.repeat)
    {
        if (m_renderMode != RenderMode::MAIN_MENU)
        {
            escapeMenuOpen = !escapeMenuOpen;
            // Release cursor when escape menu opens
            if (escapeMenuOpen) {
                SDL_SetRelativeMouseMode(SDL_FALSE);
                cursorCaptured = false;
            } else if (m_renderMode == RenderMode::WORLD) {
                // Re-capture cursor when closing in World mode
                SDL_SetRelativeMouseMode(SDL_TRUE);
                cursorCaptured = true;
            }
        }
    }

    // Tab toggles between Editor and World mode (only if escape menu is closed)
    if (event.key == events::KeyCode::Tab && !escapeMenuOpen)
    {
        // If in main menu, go to editor mode
        if (m_renderMode == RenderMode::MAIN_MENU)
        {
            m_renderMode = RenderMode::EDITOR;
            uiOpen = true;
            cursorCaptured = false;
            SDL_SetRelativeMouseMode(SDL_FALSE);
        }
        else
        {
            toggleRenderMode();
        }

        std::cout << "[Application] Switched to "
                  << (m_renderMode == RenderMode::EDITOR ? "EDITOR" : "WORLD")
                  << " mode" << std::endl;

        // In World mode, capture cursor for immersive experience
        if (m_renderMode == RenderMode::WORLD) {
            SDL_SetRelativeMouseMode(SDL_TRUE);
            cursorCaptured = true;
        } else {
            // In Editor mode, release cursor for UI interaction
            SDL_SetRelativeMouseMode(SDL_FALSE);
            cursorCaptured = false;
        }

        updateWindowTitle();
    }
}

void Application::onWindowResized(const events::WindowResizedEvent& event)
{
    renderer.handleResize(event.width, event.height);
}

void Application::onWindowClosed(const events::WindowClosedEvent& event)
{
    running = false;
}

void Application::onCameraLookRequest(const events::CameraLookRequestEvent& event)
{
    camera.handleMouseInput(event.deltaYaw, event.deltaPitch);
}

void Application::switchScene(int sceneIndex)
{
    currentSceneIndex = sceneIndex;
    if (sceneIndex == 0) {
        activeScene = &emptyScene;
        std::cout << "[Application] Switched to EmptyScene" << std::endl;
    } else {
        activeScene = &demoScene;
        std::cout << "[Application] Switched to DemoScene" << std::endl;
    }
    updateWindowTitle();
}

void Application::updateWindowTitle()
{
    std::string title = "FING-ETERNAUTA / ";

    // Add mode
    switch (m_renderMode) {
        case RenderMode::MAIN_MENU:
            title += "Main Menu";
            break;
        case RenderMode::EDITOR:
            title += "Editor - ";
            break;
        case RenderMode::WORLD:
            title += "World - ";
            break;
    }

    // Add scene name (only if not main menu)
    if (m_renderMode != RenderMode::MAIN_MENU) {
        title += (currentSceneIndex == 0) ? "EmptyScene" : "DemoScene";
    }

    renderer.setWindowTitle(title.c_str());
}
