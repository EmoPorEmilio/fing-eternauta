#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <stb_image.h>
#include <unordered_map>
#include <string>
#include <iostream>

#include "../Shader.h"
#include "../assets/AssetLoader.h"
#include "../ecs/components/Mesh.h"
#include "GameConfig.h"

// Forward declarations
struct SceneContext;

// Shader identifiers for non-RenderSystem shaders
enum class AssetShader {
    Ground,
    BuildingInstanced,
    DepthInstanced,
    Color,
    Sun,
    Comet,
    Snow,
    Depth,
    SkinnedDepth,
    MotionBlur,
    ToonPost,
    Blit,
    Overlay,
    SolidOverlay,
    DangerZone,
    RadialBlur
};

// Render target collection (FBOs + attachments)
struct RenderTargets {
    // Shadow mapping
    GLuint shadowFBO = 0;
    GLuint shadowDepthTexture = 0;

    // Motion blur
    GLuint motionBlurFBO = 0;
    GLuint motionBlurColorTex = 0;
    GLuint motionBlurDepthTex = 0;

    // Cinematic MSAA
    GLuint cinematicMsaaFBO = 0;
    GLuint cinematicMsaaColorRBO = 0;
    GLuint cinematicMsaaDepthRBO = 0;

    // Toon post-processing
    GLuint toonFBO = 0;
    GLuint toonColorTex = 0;
    GLuint toonDepthRBO = 0;

    // Main MSAA + resolve
    GLuint msaaFBO = 0;
    GLuint msaaColorRBO = 0;
    GLuint msaaDepthRBO = 0;
    GLuint resolveFBO = 0;
    GLuint resolveColorTex = 0;
};

// Primitive VAO collection
struct PrimitiveVAOs {
    GLuint planeVAO = 0;
    GLuint planeVBO = 0;
    GLuint planeEBO = 0;

    GLuint sunVAO = 0;
    GLuint sunVBO = 0;

    GLuint overlayVAO = 0;
    GLuint overlayVBO = 0;

    GLuint dangerZoneVAO = 0;
    GLuint dangerZoneVBO = 0;
};

class AssetManager {
public:
    AssetManager() = default;
    ~AssetManager() { cleanup(); }

    // Non-copyable
    AssetManager(const AssetManager&) = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    // === Main initialization ===
    bool init() {
        if (m_initialized) return true;

        loadAllTextures();
        loadAllShaders();
        loadAllModels();
        createPrimitiveVAOs();
        createRenderTargets();

        m_initialized = true;
        return true;
    }

    void cleanup() {
        if (!m_initialized) return;

        // Delete textures
        for (auto& [name, tex] : m_textures) {
            if (tex) glDeleteTextures(1, &tex);
        }
        m_textures.clear();

        // Shaders auto-cleanup via destructor
        m_shaders.clear();

        // Models cleanup (MeshGroup VAOs are owned by Registry after transfer)
        m_models.clear();

        // Primitive VAOs
        if (m_primitiveVAOs.planeVAO) glDeleteVertexArrays(1, &m_primitiveVAOs.planeVAO);
        if (m_primitiveVAOs.planeVBO) glDeleteBuffers(1, &m_primitiveVAOs.planeVBO);
        if (m_primitiveVAOs.planeEBO) glDeleteBuffers(1, &m_primitiveVAOs.planeEBO);
        if (m_primitiveVAOs.sunVAO) glDeleteVertexArrays(1, &m_primitiveVAOs.sunVAO);
        if (m_primitiveVAOs.sunVBO) glDeleteBuffers(1, &m_primitiveVAOs.sunVBO);
        if (m_primitiveVAOs.overlayVAO) glDeleteVertexArrays(1, &m_primitiveVAOs.overlayVAO);
        if (m_primitiveVAOs.overlayVBO) glDeleteBuffers(1, &m_primitiveVAOs.overlayVBO);
        if (m_primitiveVAOs.dangerZoneVAO) glDeleteVertexArrays(1, &m_primitiveVAOs.dangerZoneVAO);
        if (m_primitiveVAOs.dangerZoneVBO) glDeleteBuffers(1, &m_primitiveVAOs.dangerZoneVBO);
        m_primitiveVAOs = {};

        // Render target FBOs and attachments
        if (m_renderTargets.shadowFBO) glDeleteFramebuffers(1, &m_renderTargets.shadowFBO);
        if (m_renderTargets.shadowDepthTexture) glDeleteTextures(1, &m_renderTargets.shadowDepthTexture);

        if (m_renderTargets.motionBlurFBO) glDeleteFramebuffers(1, &m_renderTargets.motionBlurFBO);
        if (m_renderTargets.motionBlurColorTex) glDeleteTextures(1, &m_renderTargets.motionBlurColorTex);
        if (m_renderTargets.motionBlurDepthTex) glDeleteTextures(1, &m_renderTargets.motionBlurDepthTex);

        if (m_renderTargets.cinematicMsaaFBO) glDeleteFramebuffers(1, &m_renderTargets.cinematicMsaaFBO);
        if (m_renderTargets.cinematicMsaaColorRBO) glDeleteRenderbuffers(1, &m_renderTargets.cinematicMsaaColorRBO);
        if (m_renderTargets.cinematicMsaaDepthRBO) glDeleteRenderbuffers(1, &m_renderTargets.cinematicMsaaDepthRBO);

        if (m_renderTargets.toonFBO) glDeleteFramebuffers(1, &m_renderTargets.toonFBO);
        if (m_renderTargets.toonColorTex) glDeleteTextures(1, &m_renderTargets.toonColorTex);
        if (m_renderTargets.toonDepthRBO) glDeleteRenderbuffers(1, &m_renderTargets.toonDepthRBO);

        if (m_renderTargets.msaaFBO) glDeleteFramebuffers(1, &m_renderTargets.msaaFBO);
        if (m_renderTargets.msaaColorRBO) glDeleteRenderbuffers(1, &m_renderTargets.msaaColorRBO);
        if (m_renderTargets.msaaDepthRBO) glDeleteRenderbuffers(1, &m_renderTargets.msaaDepthRBO);
        if (m_renderTargets.resolveFBO) glDeleteFramebuffers(1, &m_renderTargets.resolveFBO);
        if (m_renderTargets.resolveColorTex) glDeleteTextures(1, &m_renderTargets.resolveColorTex);
        m_renderTargets = {};

        m_initialized = false;
    }

    // === Accessors ===

    Shader* getShader(AssetShader type) {
        auto it = m_shaders.find(type);
        return (it != m_shaders.end()) ? &it->second : nullptr;
    }

    GLuint getTexture(const std::string& name) const {
        auto it = m_textures.find(name);
        return (it != m_textures.end()) ? it->second : 0;
    }

    LoadedModel& getModel(const std::string& name) {
        return m_models.at(name);
    }

    MeshGroup* getMeshGroup(const std::string& name) {
        auto it = m_models.find(name);
        return (it != m_models.end()) ? &it->second.meshGroup : nullptr;
    }

    const RenderTargets& renderTargets() const { return m_renderTargets; }
    const PrimitiveVAOs& primitiveVAOs() const { return m_primitiveVAOs; }

private:
    // === Texture loading ===
    GLuint loadTextureFromFile(const std::string& path) {
        int width, height, channels;
        unsigned char* data = stbi_load(path.c_str(), &width, &height, &channels, 0);
        if (!data) {
            std::cerr << "Failed to load texture: " << path << std::endl;
            return 0;
        }

        GLuint texture;
        glGenTextures(1, &texture);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        GLenum format = (channels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);

        stbi_image_free(data);
        std::cout << "Loaded texture: " << path << " (" << width << "x" << height << ")" << std::endl;

        return texture;
    }

    void loadAllTextures() {
        m_textures["brick"] = loadTextureFromFile("assets/textures/brick/brick_wall_006_diff_1k.jpg");
        m_textures["brickNormal"] = loadTextureFromFile("assets/textures/brick/brick_wall_006_nor_gl_1k.jpg");
        m_textures["snow"] = loadTextureFromFile("assets/textures/snow.jpg");
    }

    // === Shader loading ===
    void loadAllShaders() {
        m_shaders[AssetShader::Ground].loadFromFiles("shaders/model.vert", "shaders/model.frag");
        m_shaders[AssetShader::BuildingInstanced].loadFromFiles("shaders/building_instanced.vert", "shaders/model.frag");
        m_shaders[AssetShader::DepthInstanced].loadFromFiles("shaders/depth_instanced.vert", "shaders/depth.frag");
        m_shaders[AssetShader::Color].loadFromFiles("shaders/color.vert", "shaders/color.frag");
        m_shaders[AssetShader::Sun].loadFromFiles("shaders/sun.vert", "shaders/sun.frag");
        m_shaders[AssetShader::Comet].loadFromFiles("shaders/comet.vert", "shaders/comet.frag");
        m_shaders[AssetShader::Snow].loadFromFiles("shaders/snow.vert", "shaders/snow.frag");
        m_shaders[AssetShader::Depth].loadFromFiles("shaders/depth.vert", "shaders/depth.frag");
        m_shaders[AssetShader::SkinnedDepth].loadFromFiles("shaders/skinned_depth.vert", "shaders/depth.frag");
        m_shaders[AssetShader::MotionBlur].loadFromFiles("shaders/motion_blur.vert", "shaders/motion_blur.frag");
        m_shaders[AssetShader::ToonPost].loadFromFiles("shaders/toon_post.vert", "shaders/toon_post.frag");
        m_shaders[AssetShader::Blit].loadFromFiles("shaders/fullscreen.vert", "shaders/blit.frag");
        m_shaders[AssetShader::Overlay].loadFromFiles("shaders/shadertoy_overlay.vert", "shaders/shadertoy_overlay.frag");
        m_shaders[AssetShader::SolidOverlay].loadFromFiles("shaders/solid_overlay.vert", "shaders/solid_overlay.frag");
        m_shaders[AssetShader::DangerZone].loadFromFiles("shaders/danger_zone.vert", "shaders/danger_zone.frag");
        m_shaders[AssetShader::RadialBlur].loadFromFiles("shaders/fullscreen.vert", "shaders/radial_blur.frag");
    }

    // === Model loading ===
    void loadAllModels() {
        m_models["protagonist"] = loadGLB("assets/protagonist.glb");
        m_models["fingHighDetail"] = loadGLB("assets/modelo_fing.glb");
        m_models["fingLowDetail"] = loadGLB("assets/fing_lod.glb");
        m_models["comet"] = loadGLB("assets/comet.glb");
        m_models["military"] = loadGLB("assets/military.glb");
        m_models["scientist"] = loadGLB("assets/scientist.glb");
        m_models["monster"] = loadGLB("assets/monster.glb");
    }

    // === Primitive VAO creation ===
    void createPrimitiveVAOs() {
        // Ground plane VAO
        const float planeSize = GameConfig::GROUND_SIZE;
        const float texScale = GameConfig::GROUND_TEXTURE_SCALE;
        const float uvScale = planeSize * texScale;

        float planeVertices[] = {
            // Position              // Normal       // UV
            -planeSize, 0.0f, -planeSize,  0.0f, 1.0f, 0.0f,  -uvScale, -uvScale,
             planeSize, 0.0f, -planeSize,  0.0f, 1.0f, 0.0f,   uvScale, -uvScale,
             planeSize, 0.0f,  planeSize,  0.0f, 1.0f, 0.0f,   uvScale,  uvScale,
            -planeSize, 0.0f,  planeSize,  0.0f, 1.0f, 0.0f,  -uvScale,  uvScale,
        };
        unsigned short planeIndices[] = { 0, 3, 2, 0, 2, 1 };

        glGenVertexArrays(1, &m_primitiveVAOs.planeVAO);
        glGenBuffers(1, &m_primitiveVAOs.planeVBO);
        glGenBuffers(1, &m_primitiveVAOs.planeEBO);

        glBindVertexArray(m_primitiveVAOs.planeVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_primitiveVAOs.planeVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(planeVertices), planeVertices, GL_STATIC_DRAW);
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_primitiveVAOs.planeEBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(planeIndices), planeIndices, GL_STATIC_DRAW);

        // Position (location 0)
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        // Normal (location 1)
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // UV (location 2)
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);

        // Sun billboard VAO
        float sunQuad[] = {
            -1.0f, -1.0f,
             1.0f, -1.0f,
            -1.0f,  1.0f,
             1.0f,  1.0f
        };
        glGenVertexArrays(1, &m_primitiveVAOs.sunVAO);
        glGenBuffers(1, &m_primitiveVAOs.sunVBO);
        glBindVertexArray(m_primitiveVAOs.sunVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_primitiveVAOs.sunVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(sunQuad), sunQuad, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);

        // Overlay quad VAO (fullscreen in NDC)
        float overlayQuad[] = {
            -1.0f, -1.0f,
             1.0f, -1.0f,
            -1.0f,  1.0f,
             1.0f,  1.0f
        };
        glGenVertexArrays(1, &m_primitiveVAOs.overlayVAO);
        glGenBuffers(1, &m_primitiveVAOs.overlayVBO);
        glBindVertexArray(m_primitiveVAOs.overlayVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_primitiveVAOs.overlayVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(overlayQuad), overlayQuad, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);

        // Danger zone quad VAO (unit quad in XZ plane, centered at origin)
        float dangerZoneQuad[] = {
            // Position (X, Y, Z) - Y=0, extends from -1 to 1 in X and Z
            -1.0f, 0.0f, -1.0f,
             1.0f, 0.0f, -1.0f,
            -1.0f, 0.0f,  1.0f,
             1.0f, 0.0f,  1.0f
        };
        glGenVertexArrays(1, &m_primitiveVAOs.dangerZoneVAO);
        glGenBuffers(1, &m_primitiveVAOs.dangerZoneVBO);
        glBindVertexArray(m_primitiveVAOs.dangerZoneVAO);
        glBindBuffer(GL_ARRAY_BUFFER, m_primitiveVAOs.dangerZoneVBO);
        glBufferData(GL_ARRAY_BUFFER, sizeof(dangerZoneQuad), dangerZoneQuad, GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
        glEnableVertexAttribArray(0);
        glBindVertexArray(0);
    }

    // === Render target creation ===
    void createRenderTargets() {
        const int w = GameConfig::WINDOW_WIDTH;
        const int h = GameConfig::WINDOW_HEIGHT;
        const int shadowSize = GameConfig::SHADOW_MAP_SIZE;
        const int msaaSamples = 4;

        // === Shadow FBO ===
        glGenFramebuffers(1, &m_renderTargets.shadowFBO);
        glGenTextures(1, &m_renderTargets.shadowDepthTexture);

        glBindTexture(GL_TEXTURE_2D, m_renderTargets.shadowDepthTexture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, shadowSize, shadowSize, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        float borderColor[] = {1.0f, 1.0f, 1.0f, 1.0f};
        glTexParameterfv(GL_TEXTURE_2D, GL_TEXTURE_BORDER_COLOR, borderColor);

        glBindFramebuffer(GL_FRAMEBUFFER, m_renderTargets.shadowFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_renderTargets.shadowDepthTexture, 0);
        glDrawBuffer(GL_NONE);
        glReadBuffer(GL_NONE);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // === Motion Blur FBO ===
        glGenFramebuffers(1, &m_renderTargets.motionBlurFBO);
        glGenTextures(1, &m_renderTargets.motionBlurColorTex);
        glGenTextures(1, &m_renderTargets.motionBlurDepthTex);

        // Color texture
        glBindTexture(GL_TEXTURE_2D, m_renderTargets.motionBlurColorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        // Depth texture
        glBindTexture(GL_TEXTURE_2D, m_renderTargets.motionBlurDepthTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32F, w, h, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, m_renderTargets.motionBlurFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_renderTargets.motionBlurColorTex, 0);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, m_renderTargets.motionBlurDepthTex, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Motion blur FBO is not complete!" << std::endl;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // === Cinematic MSAA FBO ===
        glGenFramebuffers(1, &m_renderTargets.cinematicMsaaFBO);
        glGenRenderbuffers(1, &m_renderTargets.cinematicMsaaColorRBO);
        glGenRenderbuffers(1, &m_renderTargets.cinematicMsaaDepthRBO);

        glBindRenderbuffer(GL_RENDERBUFFER, m_renderTargets.cinematicMsaaColorRBO);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaaSamples, GL_RGB16F, w, h);

        glBindRenderbuffer(GL_RENDERBUFFER, m_renderTargets.cinematicMsaaDepthRBO);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaaSamples, GL_DEPTH_COMPONENT32F, w, h);

        glBindFramebuffer(GL_FRAMEBUFFER, m_renderTargets.cinematicMsaaFBO);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_renderTargets.cinematicMsaaColorRBO);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, m_renderTargets.cinematicMsaaDepthRBO);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Cinematic MSAA FBO is not complete!" << std::endl;
        }
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // === Toon FBO ===
        glGenFramebuffers(1, &m_renderTargets.toonFBO);
        glGenTextures(1, &m_renderTargets.toonColorTex);
        glGenRenderbuffers(1, &m_renderTargets.toonDepthRBO);

        glBindTexture(GL_TEXTURE_2D, m_renderTargets.toonColorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindRenderbuffer(GL_RENDERBUFFER, m_renderTargets.toonDepthRBO);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);

        glBindFramebuffer(GL_FRAMEBUFFER, m_renderTargets.toonFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_renderTargets.toonColorTex, 0);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_renderTargets.toonDepthRBO);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);

        // === Main MSAA FBO ===
        glGenFramebuffers(1, &m_renderTargets.msaaFBO);
        glGenRenderbuffers(1, &m_renderTargets.msaaColorRBO);
        glGenRenderbuffers(1, &m_renderTargets.msaaDepthRBO);

        glBindRenderbuffer(GL_RENDERBUFFER, m_renderTargets.msaaColorRBO);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaaSamples, GL_RGB16F, w, h);

        glBindRenderbuffer(GL_RENDERBUFFER, m_renderTargets.msaaDepthRBO);
        glRenderbufferStorageMultisample(GL_RENDERBUFFER, msaaSamples, GL_DEPTH24_STENCIL8, w, h);

        glBindFramebuffer(GL_FRAMEBUFFER, m_renderTargets.msaaFBO);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, m_renderTargets.msaaColorRBO);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, m_renderTargets.msaaDepthRBO);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "MSAA FBO is not complete!" << std::endl;
        }

        // === Resolve FBO ===
        glGenFramebuffers(1, &m_renderTargets.resolveFBO);
        glGenTextures(1, &m_renderTargets.resolveColorTex);

        glBindTexture(GL_TEXTURE_2D, m_renderTargets.resolveColorTex);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB16F, w, h, 0, GL_RGB, GL_FLOAT, NULL);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

        glBindFramebuffer(GL_FRAMEBUFFER, m_renderTargets.resolveFBO);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_renderTargets.resolveColorTex, 0);
        if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cerr << "Resolve FBO is not complete!" << std::endl;
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        std::cout << "MSAA " << msaaSamples << "x enabled" << std::endl;
    }

    // === Storage ===
    std::unordered_map<AssetShader, Shader> m_shaders;
    std::unordered_map<std::string, GLuint> m_textures;
    std::unordered_map<std::string, LoadedModel> m_models;

    RenderTargets m_renderTargets;
    PrimitiveVAOs m_primitiveVAOs;

    bool m_initialized = false;
};
