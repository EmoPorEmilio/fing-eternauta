#include "UIManager.h"
#include "LightManager.h"
#include "Camera.h"
#include "Renderer.h"
#include "AssetManager.h"
#include "Constants.h"
#include <iostream>

UIManager& UIManager::instance() {
    static UIManager manager;
    return manager;
}

void UIManager::initialize() {
    subscribeToEvents();

    // Load initial values from ConfigManager
    auto& config = ConfigManager::instance();
    const auto& fog = config.getFog();
    m_fogEnabled = fog.enabled;
    m_fogColor[0] = fog.color.r;
    m_fogColor[1] = fog.color.g;
    m_fogColor[2] = fog.color.b;
    m_fogDensity = fog.density;
    m_fogDesaturationStrength = fog.desaturationStrength;
    m_fogAbsorptionDensity = fog.absorptionDensity;
    m_fogAbsorptionStrength = fog.absorptionStrength;

    const auto& lighting = config.getLighting();
    m_ambient = lighting.ambientIntensity;
    m_specularStrength = lighting.specularStrength;

    const auto& flashlight = config.getFlashlight();
    m_flashlightBrightness = flashlight.brightness;
    m_flashlightCutoff = flashlight.cutoffDegrees;
    m_flashlightColor[0] = flashlight.color.r;
    m_flashlightColor[1] = flashlight.color.g;
    m_flashlightColor[2] = flashlight.color.b;

    const auto& debug = config.getDebug();
    m_showGrid = debug.showGrid;
    m_showAxes = debug.showOriginAxes;

    const auto& cam = config.getCamera();
    m_moveSpeed = cam.moveSpeed;

    std::cout << "[UIManager] Initialized" << std::endl;
}

void UIManager::shutdown() {
    unsubscribeFromEvents();
    std::cout << "[UIManager] Shutdown" << std::endl;
}

void UIManager::subscribeToEvents() {
    auto& bus = events::EventBus::instance();
    m_keyPressedSub = bus.subscribe<events::KeyPressedEvent>(
        [this](const events::KeyPressedEvent& e) { onKeyPressed(e); });
}

void UIManager::unsubscribeFromEvents() {
    auto& bus = events::EventBus::instance();
    bus.unsubscribe(m_keyPressedSub);
}

void UIManager::onKeyPressed(const events::KeyPressedEvent& event) {
    // ESC toggles UI sidebar
    if (event.key == events::KeyCode::Escape && !event.repeat) {
        toggle();
    }
}

void UIManager::beginFrame() {
    // Nothing needed here for now
}

void UIManager::endFrame() {
    // Sync UI changes to ConfigManager
    auto& config = ConfigManager::instance();

    // Fog
    config.setFogEnabled(m_fogEnabled);
    config.setFogColor(glm::vec3(m_fogColor[0], m_fogColor[1], m_fogColor[2]));
    config.setFogDensity(m_fogDensity);
    config.setFogDesaturationStrength(m_fogDesaturationStrength);
    config.setFogAbsorption(m_fogAbsorptionDensity, m_fogAbsorptionStrength);

    // Lighting - use current ambient color from config
    const auto& lighting = config.getLighting();
    config.setAmbient(lighting.ambientColor, m_ambient);
    config.setSpecular(m_specularStrength, lighting.shininess);

    // Flashlight
    config.setFlashlightBrightness(m_flashlightBrightness);
    config.setFlashlightCutoff(m_flashlightCutoff);
    config.setFlashlightColor(glm::vec3(m_flashlightColor[0], m_flashlightColor[1], m_flashlightColor[2]));

    // Debug - update entire config struct
    ConfigManager::DebugConfig debug = config.getDebug();
    debug.showGrid = m_showGrid;
    debug.showOriginAxes = m_showAxes;
    config.setDebug(debug);

    // Camera - update entire config struct
    ConfigManager::CameraConfig cam = config.getCamera();
    cam.moveSpeed = m_moveSpeed;
    config.setCamera(cam);
}

bool UIManager::styledButton(const char* label, Panel panel, float width) {
    bool isActive = (m_activePanel == panel);
    if (isActive) {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.906f, 0.298f, 0.475f, 0.3f));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.906f, 0.298f, 0.475f, 1.0f));
        ImGui::PushStyleVar(ImGuiStyleVar_FrameBorderSize, 2.0f);
    }
    bool clicked = ImGui::Button(label, ImVec2(width, 42.0f));
    if (isActive) {
        ImGui::PopStyleVar();
        ImGui::PopStyleColor(2);
    }
    return clicked;
}

void UIManager::renderMenuBar(float windowWidth, float fps, bool flashlightOn) {
    float menuBarHeight = 35.0f;
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
    ImGui::TextColored(ImVec4(0.5f, 0.8f, 0.5f, 1.0f), "FPS: %.0f", fps);
    ImGui::SameLine();
    ImGui::Text("|  %s", flashlightOn ? "Flashlight ON" : "Flashlight OFF");

    ImGui::End();
    ImGui::PopStyleVar();
}

void UIManager::renderSidebar(float windowWidth, float windowHeight, float menuBarHeight,
                               LightManager& lightMgr, Camera& camera, Renderer& renderer) {
    if (!m_uiOpen) return;

    ImGui::SetNextWindowPos(ImVec2(windowWidth - m_sidebarWidth, menuBarHeight));
    ImGui::SetNextWindowSize(ImVec2(m_sidebarWidth, windowHeight - menuBarHeight));
    ImGuiWindowFlags sidebarFlags = ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                                    ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar;

    ImGui::Begin("##Sidebar", nullptr, sidebarFlags);

    // SCENE Header
    ImDrawList* drawList = ImGui::GetWindowDrawList();
    ImVec2 headerStart = ImGui::GetCursorScreenPos();
    float headerWidth = ImGui::GetContentRegionAvail().x;

    drawList->AddRectFilled(
        ImVec2(headerStart.x - 12, headerStart.y - 5),
        ImVec2(headerStart.x + headerWidth + 12, headerStart.y + 35),
        IM_COL32(46, 53, 64, 255), 8.0f);

    ImGui::Dummy(ImVec2(0, 5));
    ImGui::SetCursorPosX((headerWidth - ImGui::CalcTextSize("SCENE").x) * 0.5f);
    ImGui::TextColored(ImVec4(0.847f, 0.871f, 0.914f, 1.0f), "SCENE");
    ImGui::Dummy(ImVec2(0, 10));
    ImGui::Spacing();

    // Category buttons in 2x2 grid
    float btnWidth = (headerWidth - 15) * 0.5f;

    // Row 1: CAMERA | OBJECTS
    if (styledButton("CAMERA", Panel::Camera, btnWidth))
        m_activePanel = (m_activePanel == Panel::Camera) ? Panel::None : Panel::Camera;
    ImGui::SameLine();
    if (styledButton("OBJECTS", Panel::Objects, btnWidth))
        m_activePanel = (m_activePanel == Panel::Objects) ? Panel::None : Panel::Objects;

    ImGui::Spacing();

    // Row 2: MATERIALS | LIGHTS
    if (styledButton("MATERIALS", Panel::Materials, btnWidth))
        m_activePanel = (m_activePanel == Panel::Materials) ? Panel::None : Panel::Materials;
    ImGui::SameLine();
    if (styledButton("LIGHTS", Panel::Lights, btnWidth))
        m_activePanel = (m_activePanel == Panel::Lights) ? Panel::None : Panel::Lights;

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
        m_activePanel = (m_activePanel == Panel::Viewport) ? Panel::None : Panel::Viewport;
    ImGui::SameLine();
    if (styledButton("SYSTEM", Panel::System, btnWidth))
        m_activePanel = (m_activePanel == Panel::System) ? Panel::None : Panel::System;

    ImGui::Spacing();
    ImGui::Spacing();

    // Panel content
    if (m_activePanel != Panel::None) {
        ImGui::Separator();
        ImGui::Spacing();

        const char* panelName = "";
        switch (m_activePanel) {
            case Panel::Camera: panelName = "CAMERA"; break;
            case Panel::Objects: panelName = "OBJECTS"; break;
            case Panel::Materials: panelName = "MATERIALS"; break;
            case Panel::Lights: panelName = "LIGHTS"; break;
            case Panel::Viewport: panelName = "VIEWPORT"; break;
            case Panel::System: panelName = "SYSTEM"; break;
            default: break;
        }

        // Pink accent bar
        ImVec2 p = ImGui::GetCursorScreenPos();
        drawList->AddRectFilled(ImVec2(p.x, p.y), ImVec2(p.x + headerWidth, p.y + 3),
                                IM_COL32(231, 76, 121, 255));
        ImGui::Dummy(ImVec2(0, 8));
        ImGui::Text("%s", panelName);
        ImGui::Spacing();

        switch (m_activePanel) {
            case Panel::Camera:
                renderCameraPanel(camera);
                break;
            case Panel::Objects:
                renderObjectsPanel();
                break;
            case Panel::Materials:
                renderMaterialsPanel();
                break;
            case Panel::Lights:
                renderLightsPanel(lightMgr);
                break;
            case Panel::Viewport:
                renderViewportPanel(renderer, camera.getPosition());
                break;
            case Panel::System:
                renderSystemPanel(renderer);
                renderPerformancePanel();
                ImGui::Separator();
                renderSnowPanel();
                break;
            default: break;
        }
    }

    ImGui::End();
}

void UIManager::renderStatusBar(float windowWidth, float windowHeight, const glm::vec3& camPos) {
    float statusHeight = 28.0f;
    float sidebarAdjust = m_uiOpen ? m_sidebarWidth : 0;
    ImGui::SetNextWindowPos(ImVec2(0, windowHeight - statusHeight));
    ImGui::SetNextWindowSize(ImVec2(windowWidth - sidebarAdjust, statusHeight));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(15, 5));
    ImGui::Begin("##StatusBar", nullptr, ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize |
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoScrollbar);

    ImGui::TextColored(ImVec4(0.6f, 0.65f, 0.7f, 1.0f), "Pos: (%.1f, %.1f, %.1f)", camPos.x, camPos.y, camPos.z);
    ImGui::SameLine(200);
    ImGui::TextColored(ImVec4(0.5f, 0.55f, 0.6f, 1.0f), "ESC: Toggle Panel  |  SPACE: Flashlight  |  Right-click + drag: Look  |  WASD: Move");

    ImGui::End();
    ImGui::PopStyleVar();
}

void UIManager::renderCameraPanel(Camera& camera) {
    ImGui::Text("No models in EmptyScene");
    ImGui::Text("Use DemoScene for GLTF models");

    ImGui::Separator();
    ImGui::Text("Camera Controls");
    ImGui::SliderFloat("Move Speed", &m_moveSpeed, Constants::Camera::DEFAULT_MOVE_SPEED, Constants::Camera::MAX_MOVE_SPEED, "%.1f");
}

void UIManager::renderObjectsPanel() {
    ImGui::Text("Scene Objects");
    ImGui::Separator();
    ImGui::Text("Test Cube at origin");
    ImGui::Text("Position: (0, 1, 0)");
    ImGui::Text("Size: 2x2x2");
}

void UIManager::renderMaterialsPanel() {
    ImGui::Text("Surface Material Properties");
    ImGui::SliderFloat("Ambient", &m_ambient, 0.0f, Constants::Materials::DEFAULT_AMBIENT, "%.3f");
    ImGui::SliderFloat("Specular Strength", &m_specularStrength, 0.0f, 1.0f, "%.3f");
    ImGui::SliderFloat("Normal Strength", &m_normalStrength, 0.0f, 4.0f, "%.3f");
    ImGui::SliderFloat("Roughness Bias", &m_roughnessBias, -0.3f, 0.3f, "%.3f");

    ImGui::Separator();
    ImGui::Text("Two-Stage Fog System - TRUE Object Disappearing!");
    ImGui::Checkbox("Fog Enabled", &m_fogEnabled);
    ImGui::ColorEdit3("Fog Color (atmospheric effect)", m_fogColor);
    ImGui::SliderFloat("Fog Density (disappearing speed)", &m_fogDensity, 0.0f, 1.0f, "%.4f");
    ImGui::SliderFloat("Fog Desaturation (global effect)", &m_fogDesaturationStrength, 0.0f, 1.0f, "%.2f");
    ImGui::SliderFloat("Light Absorption Density", &m_fogAbsorptionDensity, 0.0f, 1.0f, "%.4f");
    ImGui::SliderFloat("Light Absorption Strength", &m_fogAbsorptionStrength, 0.0f, 1.0f, "%.2f");
    ImGui::Text("Objects blend to fog color, then to background for TRUE disappearing!");
}

void UIManager::renderLightsPanel(LightManager& lightMgr) {
    ImGui::Text("Flashlight Controls");
    ImGui::DragFloat("Brightness", &m_flashlightBrightness, 0.1f, 0.1f, 10.0f);
    ImGui::DragFloat("Cutoff Angle", &m_flashlightCutoff, 1.0f, 5.0f, 60.0f);
    ImGui::ColorEdit3("Color", m_flashlightColor);

    if (ImGui::Button("Toggle Flashlight (SPACE)")) {
        lightMgr.toggleFlashlight();
    }

    ImGui::Separator();
    ImGui::Text("Light Information");
    ImGui::Text("Flashlight: %s", lightMgr.isFlashlightOn() ? "ON" : "OFF");
    auto pos = lightMgr.getFlashlightPosition();
    ImGui::Text("Position: (%.1f, %.1f, %.1f)", pos.x, pos.y, pos.z);
}

void UIManager::renderViewportPanel(Renderer& renderer, const glm::vec3& camPos) {
    ImGui::Text("Blender-Style Viewport Settings");

    ImGui::Separator();
    ImGui::Text("Floor Mode");
    const char* floorModes[] = { "Grid Only", "Textured Snow", "Both" };
    ImGui::Combo("Floor Style", &m_floorMode, floorModes, IM_ARRAYSIZE(floorModes));

    ImGui::Separator();
    ImGui::Text("Debug Visualization");
    ImGui::Checkbox("Show Grid", &m_showGrid);
    if (m_showGrid) {
        ImGui::Indent();
        ImGui::SliderFloat("Grid Scale", &m_gridScale, 0.1f, 10.0f, "%.1f m");
        ImGui::SliderFloat("Fade Distance", &m_gridFadeDistance, 50.0f, 500.0f, "%.0f m");
        ImGui::Unindent();
    }

    ImGui::Checkbox("Show Origin Axes", &m_showAxes);
    ImGui::Checkbox("Show Corner Gizmo", &m_showGizmo);
    ImGui::Checkbox("Show Info Overlay", &m_showInfoOverlay);

    ImGui::Separator();
    ImGui::Text("Viewport Info");
    ImGui::Text("Resolution: %d x %d", renderer.getWidth(), renderer.getHeight());
    ImGui::Text("Camera: (%.1f, %.1f, %.1f)", camPos.x, camPos.y, camPos.z);

    if (ImGui::Button("Reset Viewport Settings")) {
        m_showGrid = true;
        m_showAxes = true;
        m_showGizmo = true;
        m_showInfoOverlay = true;
        m_gridScale = 1.0f;
        m_gridFadeDistance = 150.0f;
        m_floorMode = 0;
    }
}

void UIManager::renderSystemPanel(Renderer& renderer) {
    ImGui::Text("Scene: EmptyScene (clean 3D world)");
    ImGui::Text("Features: Floor plane + Fog system");

    ImGui::Separator();
    ImGui::Text("System Information");
    ImGui::Text("OpenGL: %s", glGetString(GL_VERSION));
    ImGui::Text("Renderer: %s", glGetString(GL_RENDERER));
    ImGui::Text("Vendor: %s", glGetString(GL_VENDOR));

    ImGui::Separator();
    ImGui::Text("Debug Options");
    ImGui::Text("OpenGL error checking: Enabled");
}

void UIManager::renderPerformancePanel() {
    ImGui::Text("Real-Time Performance Stats");
    const auto& stats = PerformanceProfiler::getCurrentFrame();
    float fps = (stats.frameTime > 0.0f) ? (1000.0f / stats.frameTime) : 0.0f;

    if (fps >= 60.0f)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.0f, 1.0f, 0.0f, 1.0f));
    else if (fps >= 30.0f)
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 1.0f, 0.0f, 1.0f));
    else
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));

    ImGui::Text("FPS: %.1f (%.2f ms)", fps, stats.frameTime);
    ImGui::PopStyleColor();

    ImGui::Text("Update: %.2f ms", stats.updateTime);
    ImGui::Text("Render: %.2f ms", stats.renderTime);

    if (ImGui::Button("Print Detailed Stats to Console")) {
        PerformanceProfiler::printStats();
    }

    ImGui::Separator();
    ImGui::Text("EmptyScene - No objects loaded");
    ImGui::Text("Use DemoScene for object presets");
}

void UIManager::renderSnowPanel() {
    ImGui::Text("No snow system in EmptyScene");
    ImGui::Text("Use DemoScene for snow particles");

    ImGui::Separator();
    ImGui::Text("Overlay Effects");
    ImGui::Checkbox("Enable Snow Overlay", &m_overlayEnabled);
    if (m_overlayEnabled) {
        ImGui::Indent();
        ImGui::DragFloat("Snow Speed", &m_overlaySnowSpeed, 0.05f, 0.0f, 8.0f, "%.2f");
        ImGui::Checkbox("Motion Blur", &m_overlayMotionBlur);
        ImGui::DragFloat("Trail Persistence", &m_overlayTrailPersistence, 0.05f, 0.0f, 10.0f, "%.2f");
        ImGui::DragFloat("Direction (deg)", &m_overlayDirectionDeg, 1.0f, 0.0f, 360.0f, "%.0f");
        ImGui::DragFloat("Trail Gain", &m_overlayTrailGain, 0.05f, 0.1f, 3.0f, "%.2f");
        ImGui::DragFloat("Advection Scale", &m_overlayAdvectionScale, 0.01f, 0.0f, 5.0f, "%.2f");
        ImGui::Unindent();
    }
}

void UIManager::applyToLightManager(LightManager& lightMgr) {
    lightMgr.setFlashlightBrightness(m_flashlightBrightness);
    lightMgr.setFlashlightColor(glm::vec3(m_flashlightColor[0], m_flashlightColor[1], m_flashlightColor[2]));
    lightMgr.setFlashlightCutoff(m_flashlightCutoff);
}
