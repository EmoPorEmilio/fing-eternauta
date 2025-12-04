/**
 * @file UIManager.h
 * @brief ImGui-based user interface management
 *
 * UIManager handles all ImGui UI rendering and state management.
 * It provides a Blender-inspired sidebar interface with collapsible panels
 * for camera, objects, materials, lights, viewport, and system settings.
 *
 * UI Layout:
 *   +------------------+-------------------+
 *   | Menu Bar         | FPS | Flashlight  |
 *   +------------------+-------------------+
 *   |                  |                   |
 *   |   3D Viewport    |     Sidebar       |
 *   |                  |   (320px wide)    |
 *   |                  | - Camera Panel    |
 *   |                  | - Objects Panel   |
 *   |                  | - Materials Panel |
 *   |                  | - Lights Panel    |
 *   |                  | - Viewport Panel  |
 *   |                  | - System Panel    |
 *   +------------------+-------------------+
 *   | Status Bar (camera position)         |
 *   +--------------------------------------+
 *
 * State Management:
 *   UIManager maintains local copies of settings (m_fog*, m_snow*, etc.)
 *   which are synced with ConfigManager. This is a known architectural
 *   issue - ideally UIManager should read directly from ConfigManager.
 *
 * Event Integration:
 *   - Subscribes to KeyPressedEvent for keyboard shortcuts
 *   - Calls ConfigManager setters which publish change events
 *
 * Keyboard Shortcuts:
 *   - Tab: Toggle UI visibility
 *   - Esc: Release cursor / close panels
 *   - L: Toggle flashlight
 *
 * @see ConfigManager, Application, LightManager
 */
#pragma once

#include <SDL.h>
#include <glm/glm.hpp>
#include "events/Events.h"
#include "ConfigManager.h"
#include "libraries/imgui/imgui.h"

// Forward declarations
class LightManager;
class Camera;
class Renderer;

class UIManager {
public:
    static UIManager& instance();

    // Initialize/shutdown
    void initialize();
    void shutdown();

    // Panel state
    bool isOpen() const { return m_uiOpen; }
    void setOpen(bool open) { m_uiOpen = open; }
    void toggle() { m_uiOpen = !m_uiOpen; }

    // Panel navigation
    enum class Panel { None, Camera, Objects, Materials, Lights, Viewport, System };
    Panel getActivePanel() const { return m_activePanel; }
    void setActivePanel(Panel panel) { m_activePanel = panel; }

    // Rendering - called from Application
    void beginFrame();
    void endFrame();

    // Render specific UI sections
    void renderMenuBar(float windowWidth, float fps, bool flashlightOn);
    void renderSidebar(float windowWidth, float windowHeight, float menuBarHeight,
                       LightManager& lightMgr, Camera& camera, Renderer& renderer);
    void renderStatusBar(float windowWidth, float windowHeight, const glm::vec3& camPos);

    // Panel content renderers
    void renderCameraPanel(Camera& camera);
    void renderObjectsPanel();
    void renderMaterialsPanel();
    void renderLightsPanel(LightManager& lightMgr);
    void renderViewportPanel(Renderer& renderer, const glm::vec3& camPos);
    void renderSystemPanel(Renderer& renderer);
    void renderPerformancePanel();
    void renderSnowPanel();

    // Apply UI state to systems (called each frame)
    void applyToLightManager(LightManager& lightMgr);

    // Getters for values that need direct access
    float getMoveSpeed() const { return m_moveSpeed; }
    bool isOverlayEnabled() const { return m_overlayEnabled; }
    float getOverlaySnowSpeed() const { return m_overlaySnowSpeed; }
    bool isOverlayMotionBlur() const { return m_overlayMotionBlur; }
    float getOverlayTrailPersistence() const { return m_overlayTrailPersistence; }
    float getOverlayDirectionDeg() const { return m_overlayDirectionDeg; }
    float getOverlayTrailGain() const { return m_overlayTrailGain; }
    float getOverlayAdvectionScale() const { return m_overlayAdvectionScale; }

private:
    UIManager() = default;
    ~UIManager() = default;
    UIManager(const UIManager&) = delete;
    UIManager& operator=(const UIManager&) = delete;

    // Event handling
    void subscribeToEvents();
    void unsubscribeFromEvents();
    void onKeyPressed(const events::KeyPressedEvent& event);
    events::SubscriptionId m_keyPressedSub = 0;

    // UI state
    bool m_uiOpen = true;
    Panel m_activePanel = Panel::None;
    float m_sidebarWidth = 320.0f;

    // Material settings (sync with ConfigManager)
    float m_ambient = 0.2f;
    float m_specularStrength = 0.5f;
    float m_normalStrength = 0.276f;
    float m_roughnessBias = 0.0f;

    // Model transforms (for DemoScene)
    // Walking model (model_Animation_Walking_withSkin.glb)
    bool m_walkingEnabled = true;
    float m_walkingPos[3] = {-3.0f, 0.0f, -5.0f};
    float m_walkingScale = 1000.0f;
    bool m_walkingAnim = true;
    float m_walkingAnimSpeed = 1.0f;

    // Monster-2 model (monster-2.glb)
    bool m_monster2Enabled = true;
    float m_monster2Pos[3] = {3.0f, 0.0f, -5.0f};
    float m_monster2Scale = 1000.0f;
    bool m_monster2Anim = true;
    float m_monster2AnimSpeed = 1.0f;

    // Flashlight (sync with ConfigManager)
    float m_flashlightBrightness = 2.0f;
    float m_flashlightCutoff = 25.0f;
    float m_flashlightColor[3] = {1.0f, 0.8f, 0.6f};

    // Snow settings
    bool m_snowEnabled = true;
    int m_snowCount = 30000;
    float m_snowFallSpeed = 10.0f;
    float m_snowWindSpeed = 5.0f;
    float m_snowWindDirection = 180.0f;
    float m_snowSpriteSize = 0.05f;
    float m_snowTimeScale = 1.0f;
    bool m_snowBulletGround = true;
    bool m_snowFrustumCulling = true;
    bool m_snowLOD = true;
    int m_snowMaxVisible = 100000;

    // Object culling
    bool m_objectCulling = true;
    bool m_objectLOD = false;

    // Fog settings (sync with ConfigManager)
    bool m_fogEnabled = true;
    float m_fogColor[3] = {0.0667f, 0.0784f, 0.0980f};
    float m_fogDensity = 0.0050f;
    float m_fogDesaturationStrength = 0.79f;
    float m_fogAbsorptionDensity = 0.0427f;
    float m_fogAbsorptionStrength = 1.0f;

    // Camera settings
    float m_moveSpeed = 30.0f;

    // Overlay settings
    bool m_overlayEnabled = false;
    float m_overlaySnowSpeed = 8.0f;
    bool m_overlayMotionBlur = true;
    float m_overlayTrailPersistence = 5.55f;
    float m_overlayDirectionDeg = 162.0f;
    float m_overlayTrailGain = 3.0f;
    float m_overlayAdvectionScale = 3.25f;

    // Viewport settings (sync with ConfigManager)
    bool m_showGrid = true;
    bool m_showAxes = true;
    bool m_showGizmo = true;
    bool m_showInfoOverlay = true;
    float m_gridScale = 1.0f;
    float m_gridFadeDistance = 150.0f;
    int m_floorMode = 0;

    // Helper for styled buttons
    bool styledButton(const char* label, Panel panel, float width);
};
