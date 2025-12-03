#include "BaseScene.h"
#include "LightManager.h"
#include <glm/gtc/matrix_transform.hpp>
#include <iostream>

BaseScene::BaseScene()
{
}

BaseScene::~BaseScene()
{
    cleanup();
}

bool BaseScene::initialize()
{
    // Subscribe to config events
    subscribeToEvents();

    // Initialize debug renderer first
    if (!m_debugRenderer.initialize()) {
        std::cerr << "BaseScene: Failed to initialize debug renderer" << std::endl;
        return false;
    }

    setupFloorGeometry();
    setupFloorShader();

    // Load floor textures
    // Albedo is color data -> sRGB; roughness/translucency/height are data -> linear
    bool ok = true;
    ok = ok && m_albedoTex.loadFromFile("snow/snow_02_diff_1k.jpg", true, true);
    ok = ok && m_roughnessTex.loadFromFile("snow/snow_02_rough_1k.jpg", true, false);
    ok = ok && m_translucencyTex.loadFromFile("snow/snow_02_translucent_1k.png", true, false);
    ok = ok && m_heightTex.loadFromFile("snow/snow_02_disp_1k.png", true, false);

    if (!ok) {
        std::cerr << "BaseScene: Failed to load floor textures" << std::endl;
        return false;
    }

    std::cout << "BaseScene: Initialized with Blender-style debug viewport" << std::endl;
    return true;
}

void BaseScene::update(const glm::vec3& /*cameraPos*/, float /*deltaTime*/,
                       const glm::mat4& /*viewMatrix*/, const glm::mat4& /*projectionMatrix*/)
{
    // Base scene has nothing to update - derived classes override this
}

void BaseScene::render(const glm::mat4& view, const glm::mat4& projection,
                       const glm::vec3& cameraPos, const glm::vec3& cameraFront,
                       LightManager& lightManager)
{
    // Render textured floor based on floor mode
    if (m_floorMode == FloorMode::TexturedSnow || m_floorMode == FloorMode::Both) {
        renderFloor(view, projection, cameraPos, cameraFront, lightManager);
    }

    // Render debug visualization (grid, axes, gizmo)
    // Only render grid if floor mode includes it
    bool wasGridEnabled = m_debugRenderer.isGridEnabled();
    if (m_floorMode == FloorMode::TexturedSnow) {
        m_debugRenderer.setGridEnabled(false);
    }

    m_debugRenderer.render(view, projection, cameraPos, cameraFront,
                           m_viewportWidth, m_viewportHeight);

    // Restore grid state
    m_debugRenderer.setGridEnabled(wasGridEnabled);
}

void BaseScene::renderFloor(const glm::mat4& view, const glm::mat4& projection,
                            const glm::vec3& cameraPos, const glm::vec3& cameraFront,
                            LightManager& lightManager)
{
    m_floorShader.use();

    // Set uniforms for floor
    m_floorShader.setUniform("uModel", glm::mat4(1.0f));
    m_floorShader.setUniform("uView", view);
    m_floorShader.setUniform("uProj", projection);
    m_floorShader.setUniform("uLightPos", glm::vec3(2.0f, 4.0f, 2.0f));
    m_floorShader.setUniform("uViewPos", cameraPos);
    m_floorShader.setUniform("uLightColor", glm::vec3(1.0f, 1.0f, 1.0f));
    m_floorShader.setUniform("uObjectColor", glm::vec3(1.0f, 1.0f, 1.0f));
    m_floorShader.setUniform("uCullDistance", 400.0f);
    m_floorShader.setUniform("uAmbient", m_ambient);
    m_floorShader.setUniform("uSpecularStrength", m_specularStrength);

    // Set flashlight uniforms for floor
    m_floorShader.setUniform("uFlashlightOn", lightManager.isFlashlightOn());
    m_floorShader.setUniform("uFlashlightPos", cameraPos);
    m_floorShader.setUniform("uFlashlightDir", cameraFront);
    m_floorShader.setUniform("uFlashlightCutoff", lightManager.getFlashlightCutoff());
    m_floorShader.setUniform("uFlashlightBrightness", lightManager.getFlashlightBrightness());
    m_floorShader.setUniform("uFlashlightColor", lightManager.getFlashlightColor());

    // Set fog uniforms for floor
    m_floorShader.setUniform("uFogEnabled", m_fogEnabled);
    m_floorShader.setUniform("uFogColor", m_fogColor);
    m_floorShader.setUniform("uFogDensity", m_fogDensity);
    m_floorShader.setUniform("uFogDesaturationStrength", m_fogDesaturationStrength);
    m_floorShader.setUniform("uFogAbsorptionDensity", m_fogAbsorptionDensity);
    m_floorShader.setUniform("uFogAbsorptionStrength", m_fogAbsorptionStrength);
    m_floorShader.setUniform("uBackgroundColor", glm::vec3(0.08f, 0.1f, 0.12f));

    // Bind textures
    m_albedoTex.bind(0);
    m_floorShader.setUniform("uAlbedoTex", 0);
    m_roughnessTex.bind(1);
    m_floorShader.setUniform("uRoughnessTex", 1);
    m_translucencyTex.bind(2);
    m_floorShader.setUniform("uTranslucencyTex", 2);
    m_heightTex.bind(3);
    m_floorShader.setUniform("uHeightTex", 3);
    m_floorShader.setUniform("uNormalStrength", m_normalStrength);
    m_floorShader.setUniform("uWorldPerUV", glm::vec2(10.0f, 10.0f));
    m_floorShader.setUniform("uRoughnessBias", m_roughnessBias);

    // Draw floor
    glBindVertexArray(m_floorVAO);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glBindVertexArray(0);
}

void BaseScene::cleanup()
{
    // Unsubscribe from events first
    unsubscribeFromEvents();

    m_debugRenderer.cleanup();

    if (m_floorVAO != 0) {
        glDeleteVertexArrays(1, &m_floorVAO);
        m_floorVAO = 0;
    }
    if (m_floorVBO != 0) {
        glDeleteBuffers(1, &m_floorVBO);
        m_floorVBO = 0;
    }
}

void BaseScene::setupFloorGeometry()
{
    // Floor plane (Y=0) sized 2000x2000 with heavy UV tiling
    float vertices[] = {
        // position              normal         uv (tile 200x200)
        -1000.0f, 0.0f, -1000.0f,  0.0f, 1.0f, 0.0f,    0.0f,   0.0f,
         1000.0f, 0.0f, -1000.0f,  0.0f, 1.0f, 0.0f,  200.0f,   0.0f,
         1000.0f, 0.0f,  1000.0f,  0.0f, 1.0f, 0.0f,  200.0f, 200.0f,

        -1000.0f, 0.0f, -1000.0f,  0.0f, 1.0f, 0.0f,    0.0f,   0.0f,
         1000.0f, 0.0f,  1000.0f,  0.0f, 1.0f, 0.0f,  200.0f, 200.0f,
        -1000.0f, 0.0f,  1000.0f,  0.0f, 1.0f, 0.0f,    0.0f, 200.0f,
    };

    glGenVertexArrays(1, &m_floorVAO);
    glGenBuffers(1, &m_floorVBO);

    glBindVertexArray(m_floorVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_floorVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    // Normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    // UV
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));

    glBindVertexArray(0);
}

void BaseScene::setupFloorShader()
{
    m_floorShader.loadFromFiles("phong_notess.vert", "phong_notess.frag");
}

// ==================== Event Handling ====================

void BaseScene::subscribeToEvents()
{
    auto& bus = events::EventBus::instance();

    m_fogSubscription = bus.subscribe<events::FogConfigChangedEvent>(
        [this](const events::FogConfigChangedEvent& e) { onFogConfigChanged(e); });

    m_materialSubscription = bus.subscribe<events::MaterialConfigChangedEvent>(
        [this](const events::MaterialConfigChangedEvent& e) { onMaterialConfigChanged(e); });

    m_debugVisualsSubscription = bus.subscribe<events::DebugVisualsChangedEvent>(
        [this](const events::DebugVisualsChangedEvent& e) { onDebugVisualsChanged(e); });

    std::cout << "[BaseScene] Subscribed to config events" << std::endl;
}

void BaseScene::unsubscribeFromEvents()
{
    auto& bus = events::EventBus::instance();

    if (m_fogSubscription != 0) {
        bus.unsubscribe(m_fogSubscription);
        m_fogSubscription = 0;
    }
    if (m_materialSubscription != 0) {
        bus.unsubscribe(m_materialSubscription);
        m_materialSubscription = 0;
    }
    if (m_debugVisualsSubscription != 0) {
        bus.unsubscribe(m_debugVisualsSubscription);
        m_debugVisualsSubscription = 0;
    }
}

void BaseScene::onFogConfigChanged(const events::FogConfigChangedEvent& event)
{
    m_fogEnabled = event.enabled;
    m_fogColor = event.color;
    m_fogDensity = event.density;
    m_fogDesaturationStrength = event.desaturationStrength;
    m_fogAbsorptionDensity = event.absorptionDensity;
    m_fogAbsorptionStrength = event.absorptionStrength;
}

void BaseScene::onMaterialConfigChanged(const events::MaterialConfigChangedEvent& event)
{
    m_ambient = event.ambient;
    m_specularStrength = event.specularStrength;
    m_normalStrength = event.normalStrength;
    m_roughnessBias = event.roughnessBias;
}

void BaseScene::onDebugVisualsChanged(const events::DebugVisualsChangedEvent& event)
{
    m_debugRenderer.setGridEnabled(event.showGrid);
    m_debugRenderer.setAxesEnabled(event.showOriginAxes);
    m_debugRenderer.setGizmoEnabled(event.showGizmo);
    m_debugRenderer.setGridScale(event.gridScale);
    m_debugRenderer.setGridFadeDistance(event.gridFadeDistance);

    // Floor mode conversion
    switch (event.floorMode) {
        case 0: m_floorMode = FloorMode::GridOnly; break;
        case 1: m_floorMode = FloorMode::TexturedSnow; break;
        case 2: m_floorMode = FloorMode::Both; break;
        default: m_floorMode = FloorMode::GridOnly; break;
    }
}
