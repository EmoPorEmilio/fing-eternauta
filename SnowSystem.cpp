#include "SnowSystem.h"
#include "Shader.h"
#include <iostream>
#include <algorithm>
#include <fstream>
#include <glm/gtc/constants.hpp>

// Require Bullet at build time (no fallback)
#include <bullet/btBulletDynamicsCommon.h>

SnowSystem::SnowSystem()
    : m_enabled(true), m_initialized(false), m_count(30000), m_fallSpeed(10.0f), m_windSpeed(5.0f), m_windDirection(glm::radians(180.0f)), m_spriteSize(0.05f), m_timeScale(1.0f), m_accumulatedTime(0.0f), m_spawnHeight(50.0f), m_spawnRadius(100.0f), m_floorY(0.0f), m_frustumCulling(true), m_visibleCount(0), m_drawCount(0), m_fogEnabled(true), m_fogColor(0.0f, 0.0f, 0.0f), m_fogDensity(0.01f), m_fogDesaturationStrength(1.0f), m_fogAbsorptionDensity(0.02f), m_fogAbsorptionStrength(0.8f), m_quadVAO(0), m_quadVBO(0), m_instanceVBO(0), m_puffVAO(0), m_puffInstanceVBO(0), m_shader(nullptr), m_rng(std::random_device{}()), m_uniformDist(0.0f, 1.0f)
{
    // Bullet members
    m_bulletEnabled = false;
    m_bulletCollisionConfig = nullptr;
    m_bulletDispatcher = nullptr;
    m_bulletBroadphase = nullptr;
    m_bulletSolver = nullptr;
    m_bulletWorld = nullptr;
    m_groundShape = nullptr;
    m_groundMotionState = nullptr;
    m_groundBody = nullptr;
}

SnowSystem::~SnowSystem()
{
    shutdown();
}

bool SnowSystem::initialize()
{
    if (m_initialized)
    {
        return true;
    }

    std::cout << "[SnowSystem] Initializing with " << m_count << " snowflakes..." << std::endl;

    // Minimal diagnostic without C++17 deps
    std::cout << "[SnowSystem] Looking for snow_glow.vert/.frag via Shader loader" << std::endl;

    // Create shader using filename-only; loader resolves multiple directories
    m_shader = new Shader();
    if (!m_shader->loadFromFiles("snow_glow.vert", "snow_glow.frag"))
    {
        std::cout << "[SnowSystem] Failed to load snow shaders!" << std::endl;
        delete m_shader;
        m_shader = nullptr;
        return false;
    }

    // Setup quad geometry (simple 2D quad for billboards)
    float quadVertices[] = {
        // positions
        -1.0f, 1.0f, 0.0f,
        -1.0f, -1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        -1.0f, 1.0f, 0.0f,
        1.0f, -1.0f, 0.0f,
        1.0f, 1.0f, 0.0f};

    glGenVertexArrays(1, &m_quadVAO);
    glGenBuffers(1, &m_quadVBO);
    glGenBuffers(1, &m_instanceVBO);
    glGenVertexArrays(1, &m_puffVAO);
    glGenBuffers(1, &m_puffInstanceVBO);

    glBindVertexArray(m_quadVAO);

    // Quad vertices
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

    // Instance data (position + seed)
    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glVertexAttribDivisor(1, 1);

    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 1, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)(3 * sizeof(float)));
    glVertexAttribDivisor(2, 1);

    glBindVertexArray(0);

    // Puff instancing: reuse same quad vertex buffer, separate VAO with instance positions (xyz) + age (w)
    glBindVertexArray(m_puffVAO);
    glBindBuffer(GL_ARRAY_BUFFER, m_quadVBO);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glBindBuffer(GL_ARRAY_BUFFER, m_puffInstanceVBO);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void *)0);
    glVertexAttribDivisor(1, 1);
    glBindVertexArray(0);

    // Initialize snowflakes with randomized positions to avoid vertical columns
    m_snowflakes.resize(m_count);
    for (int i = 0; i < m_count; ++i)
    {
        respawnFlake(i);
    }

    updateBuffers();

    m_initialized = true;
    std::cout << "[SnowSystem] Initialized successfully!" << std::endl;
    if (m_bulletEnabled)
    {
        std::cout << "[SnowSystem][Bullet] Bullet world will be initialized on first toggle." << std::endl;
    }
    return true;
}

void SnowSystem::shutdown()
{
    if (!m_initialized)
    {
        return;
    }

    // Shutdown Bullet if active
    shutdownBullet();

    if (m_quadVAO)
    {
        glDeleteVertexArrays(1, &m_quadVAO);
        m_quadVAO = 0;
    }
    if (m_quadVBO)
    {
        glDeleteBuffers(1, &m_quadVBO);
        m_quadVBO = 0;
    }
    if (m_instanceVBO)
    {
        glDeleteBuffers(1, &m_instanceVBO);
        m_instanceVBO = 0;
    }
    if (m_puffInstanceVBO)
    {
        glDeleteBuffers(1, &m_puffInstanceVBO);
        m_puffInstanceVBO = 0;
    }
    if (m_puffVAO)
    {
        glDeleteVertexArrays(1, &m_puffVAO);
        m_puffVAO = 0;
    }

    if (m_shader)
    {
        delete m_shader;
        m_shader = nullptr;
    }

    m_initialized = false;
    std::cout << "[SnowSystem] Shutdown complete" << std::endl;
}

void SnowSystem::update(float deltaTime, const glm::vec3 &cameraPos, const glm::mat4 &viewMatrix, const glm::mat4 &projectionMatrix)
{
    if (!m_enabled || !m_initialized)
    {
        return;
    }

    m_accumulatedTime += deltaTime * m_timeScale;

    // Calculate wind direction vector
    glm::vec3 windDir = glm::vec3(
        glm::cos(m_windDirection),
        0.0f,
        glm::sin(m_windDirection));

    // Update each snowflake
    for (int i = 0; i < m_count; ++i)
    {
        Snowflake &flake = m_snowflakes[i];

        // Store previous position for motion blur (future use)
        flake.prevPosition = flake.position;

        // Apply gravity and wind
        glm::vec3 velocity = glm::vec3(0.0f, -flake.fallSpeed * m_fallSpeed, 0.0f);
        velocity += windDir * m_windSpeed * (0.5f + 0.5f * flake.seed); // Wind variation per flake

        // Add some sway based on position and time
        float sway = glm::sin(flake.position.x * 0.1f + m_accumulatedTime * 2.0f) * 0.1f;
        velocity.x += sway * (0.3f + 0.7f * flake.seed);

        // If flake is settled, count down and then respawn
        if (flake.settled)
        {
            flake.settleTimer -= deltaTime;
            if (flake.settleTimer <= 0.0f)
            {
                respawnFlake(i);
            }
            continue;
        }

        glm::vec3 nextPosition = flake.position + velocity * deltaTime;

        if (m_bulletEnabled && m_bulletWorld)
        {
            // Raycast from previous to next to detect ground hit
            btVector3 from(flake.prevPosition.x, flake.prevPosition.y, flake.prevPosition.z);
            btVector3 to(nextPosition.x, nextPosition.y, nextPosition.z);
            btCollisionWorld::ClosestRayResultCallback cb(from, to);
            m_bulletWorld->rayTest(from, to, cb);
            if (cb.hasHit())
            {
                // Place at hit point (slightly above) and respawn up top to keep density
                btVector3 hit = cb.m_hitPointWorld;
                flake.position = glm::vec3(nextPosition.x, static_cast<float>(hit.getY()) + 0.002f, nextPosition.z);
                // Enter settled state briefly and spawn a puff
                flake.settled = true;
                flake.settleTimer = m_settleDuration;
                m_puffs.push_back({flake.position, 0.0f, m_puffLifetime});
            }
            else
            {
                flake.position = nextPosition;
                if (flake.position.y < m_floorY)
                {
                    // Fallback clamp/respawn if plane missed
                    flake.position.y = m_floorY + 0.002f;
                    flake.settled = true;
                    flake.settleTimer = m_settleDuration;
                    m_puffs.push_back({flake.position, 0.0f, m_puffLifetime});
                }
            }
        }
        else
        {
            flake.position = nextPosition;
            // Respawn if fallen below floor or NaN/Inf
            if (flake.position.y < m_floorY)
            {
                flake.position.y = m_floorY + 0.002f;
                flake.settled = true;
                flake.settleTimer = m_settleDuration;
                m_puffs.push_back({flake.position, 0.0f, m_puffLifetime});
            }
        }
    }

    // Perform frustum culling if enabled
    if (m_frustumCulling)
    {
        glm::mat4 viewProj = projectionMatrix * viewMatrix;
        performFrustumCulling(viewProj);
    }
    else
    {
        m_visibleCount = m_count;
        m_visibleIndices.resize(m_count);
        for (int i = 0; i < m_count; ++i)
        {
            m_visibleIndices[i] = i;
        }
    }

    updateBuffers();
    updatePuffs(deltaTime);
}

void SnowSystem::setBulletGroundCollisionEnabled(bool enabled)
{
    if (enabled == m_bulletEnabled)
    {
        return;
    }
    m_bulletEnabled = enabled;
    if (m_bulletEnabled)
    {
        std::cout << "[SnowSystem][Bullet] Enabling Bullet ground collision..." << std::endl;
        initializeBullet();
    }
    else
    {
        std::cout << "[SnowSystem][Bullet] Disabling Bullet ground collision..." << std::endl;
        shutdownBullet();
    }
}

void SnowSystem::render(const glm::mat4 &view, const glm::mat4 &projection, const glm::vec3 &cameraPos)
{
    if (!m_enabled || !m_initialized || !m_shader)
    {
        return;
    }

    // Enable alpha blending for transparent snow quads and disable depth writes
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glDepthMask(GL_FALSE);

    m_shader->use();

    // Set uniforms
    m_shader->setUniform("projection", projection);
    m_shader->setUniform("view", view);
    m_shader->setUniform("model", glm::mat4(1.0f));
    m_shader->setUniform("useBillboard", 1);
    m_shader->setUniform("writeVelocity", 0);
    m_shader->setUniform("spriteSize", m_spriteSize);
    m_shader->setUniform("time", m_accumulatedTime);
    m_shader->setUniform("windStrength", 0.3f);
    m_shader->setUniform("windFrequency", 1.0f);
    m_shader->setUniform("windDir", glm::vec3(glm::cos(m_windDirection), 0.0f, glm::sin(m_windDirection)));

    // Simple material settings (start with basic white flakes)
    m_shader->setUniform("useDisc", 0); // Simple quads for now
    m_shader->setUniform("glowIntensity", 0.0f);
    m_shader->setUniform("sparkleIntensity", 0.0f);
    m_shader->setUniform("baseAlpha", 0.8f);
    m_shader->setUniform("trailOpacity", 1.0f);

    // Set fog uniforms
    m_shader->setUniform("uFogEnabled", m_fogEnabled);
    m_shader->setUniform("uFogColor", m_fogColor);
    m_shader->setUniform("uFogDensity", m_fogDensity);
    m_shader->setUniform("uFogAbsorptionDensity", m_fogAbsorptionDensity);
    m_shader->setUniform("uFogAbsorptionStrength", m_fogAbsorptionStrength);
    m_shader->setUniform("uFogDesaturationStrength", m_fogDesaturationStrength);
    m_shader->setUniform("uBackgroundColor", glm::vec3(0.08f, 0.1f, 0.12f)); // Match renderer clear color
    m_shader->setUniform("uCameraPos", cameraPos);

    // Render instanced quads (with culling if enabled)
    glBindVertexArray(m_quadVAO);
    glDrawArraysInstanced(GL_TRIANGLES, 0, 6, m_visibleCount);
    glBindVertexArray(0);

    // Restore default depth write and blending state
    glDepthMask(GL_TRUE);
    glDisable(GL_BLEND);

    // Render impact puffs as fading discs
    if (!m_puffs.empty())
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
        glDepthMask(GL_FALSE);

        m_shader->use();
        m_shader->setUniform("projection", projection);
        m_shader->setUniform("view", view);
        m_shader->setUniform("model", glm::mat4(1.0f));
        m_shader->setUniform("useBillboard", 1);
        m_shader->setUniform("writeVelocity", 0);
        m_shader->setUniform("spriteSize", m_puffSize);
        m_shader->setUniform("time", m_accumulatedTime);
        m_shader->setUniform("windStrength", 0.0f);
        m_shader->setUniform("windFrequency", 0.0f);
        m_shader->setUniform("windDir", glm::vec3(0.0f));

        // Fog uniforms for puffs
        m_shader->setUniform("uFogEnabled", m_fogEnabled);
        m_shader->setUniform("uFogColor", m_fogColor);
        m_shader->setUniform("uFogDensity", m_fogDensity);
        m_shader->setUniform("uFogAbsorptionDensity", m_fogAbsorptionDensity);
        m_shader->setUniform("uFogAbsorptionStrength", m_fogAbsorptionStrength);
        m_shader->setUniform("uFogDesaturationStrength", m_fogDesaturationStrength);
        m_shader->setUniform("uBackgroundColor", glm::vec3(0.08f, 0.1f, 0.12f)); // Match renderer clear color
        m_shader->setUniform("uCameraPos", cameraPos);

        uploadPuffs();
        glBindVertexArray(m_puffVAO);
        // Draw each puff with its own instanceAge via uniform (update per puff)
        for (size_t i = 0; i < m_puffs.size(); ++i)
        {
            float ageN = m_puffs[i].lifetime > 0.0f ? (m_puffs[i].age / m_puffs[i].lifetime) : 1.0f;
            m_shader->setUniform("instanceAge", ageN);
            glDrawArrays(GL_TRIANGLES, 0, 6);
        }
        glBindVertexArray(0);

        glDepthMask(GL_TRUE);
        glDisable(GL_BLEND);
    }
}

void SnowSystem::setCount(int count)
{
    count = std::max(0, std::min(count, 50000)); // Clamp to reasonable range
    if (count == m_count)
    {
        return;
    }

    m_count = count;
    m_snowflakes.resize(m_count);

    // Reinitialize snowflakes
    for (int i = 0; i < m_count; ++i)
    {
        m_snowflakes[i].position = glm::vec3(0.0f, m_spawnHeight, 0.0f);
        m_snowflakes[i].prevPosition = m_snowflakes[i].position;
        m_snowflakes[i].seed = m_uniformDist(m_rng);
        m_snowflakes[i].fallSpeed = 0.5f + m_uniformDist(m_rng) * 1.5f;
    }

    if (m_initialized)
    {
        updateBuffers();
    }
}

void SnowSystem::updateBuffers()
{
    if (!m_initialized)
    {
        return;
    }

    // Update instance buffer with current positions and seeds (visible flakes only)
    std::vector<float> instanceData(m_visibleCount * 4); // 3 for position + 1 for seed
    for (int i = 0; i < m_visibleCount; ++i)
    {
        int flakeIndex = m_visibleIndices[i];
        instanceData[i * 4 + 0] = m_snowflakes[flakeIndex].position.x;
        instanceData[i * 4 + 1] = m_snowflakes[flakeIndex].position.y;
        instanceData[i * 4 + 2] = m_snowflakes[flakeIndex].position.z;
        instanceData[i * 4 + 3] = m_snowflakes[flakeIndex].seed;
    }

    glBindBuffer(GL_ARRAY_BUFFER, m_instanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(float), instanceData.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void SnowSystem::updatePuffs(float deltaTime)
{
    if (m_puffs.empty())
    {
        return;
    }
    for (auto &p : m_puffs)
    {
        p.age += deltaTime;
    }
    // Remove expired
    m_puffs.erase(std::remove_if(m_puffs.begin(), m_puffs.end(), [](const ImpactPuff &p)
                                 { return p.age >= p.lifetime; }),
                  m_puffs.end());
}

void SnowSystem::uploadPuffs()
{
    if (m_puffs.empty())
    {
        return;
    }
    // pack as vec4: xyz pos + age(normalized 0..1)
    std::vector<float> instanceData(m_puffs.size() * 4);
    for (size_t i = 0; i < m_puffs.size(); ++i)
    {
        const auto &p = m_puffs[i];
        instanceData[i * 4 + 0] = p.position.x;
        instanceData[i * 4 + 1] = p.position.y + 0.001f;
        instanceData[i * 4 + 2] = p.position.z;
        instanceData[i * 4 + 3] = p.lifetime > 0.0f ? (p.age / p.lifetime) : 1.0f;
    }
    glBindBuffer(GL_ARRAY_BUFFER, m_puffInstanceVBO);
    glBufferData(GL_ARRAY_BUFFER, instanceData.size() * sizeof(float), instanceData.data(), GL_DYNAMIC_DRAW);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
}

void SnowSystem::respawnFlake(int index)
{
    if (index < 0 || index >= m_count)
    {
        return;
    }

    Snowflake &flake = m_snowflakes[index];

    // Respawn above camera with some randomness
    flake.position = glm::vec3(
        (m_uniformDist(m_rng) - 0.5f) * m_spawnRadius * 2.0f,
        m_spawnHeight + m_uniformDist(m_rng) * 20.0f,
        (m_uniformDist(m_rng) - 0.5f) * m_spawnRadius * 2.0f);

    flake.prevPosition = flake.position;
    flake.seed = m_uniformDist(m_rng);
    flake.fallSpeed = 0.5f + m_uniformDist(m_rng) * 1.5f;
}

glm::vec3 SnowSystem::getRandomSpawnPosition(const glm::vec3 &cameraPos, const glm::mat4 &viewMatrix)
{
    // Spawn in a circle around camera, above
    float angle = m_uniformDist(m_rng) * 2.0f * glm::pi<float>();
    float radius = m_uniformDist(m_rng) * m_spawnRadius;

    return glm::vec3(
        cameraPos.x + glm::cos(angle) * radius,
        cameraPos.y + m_spawnHeight + m_uniformDist(m_rng) * 20.0f,
        cameraPos.z + glm::sin(angle) * radius);
}

void SnowSystem::performFrustumCulling(const glm::mat4 &viewProj)
{
    m_visibleIndices.clear();
    m_visibleCount = 0;

    // Extract frustum planes from view-projection matrix (GLM is column-major)
    // Build row vectors explicitly for readability
    const glm::vec4 row0(viewProj[0][0], viewProj[1][0], viewProj[2][0], viewProj[3][0]);
    const glm::vec4 row1(viewProj[0][1], viewProj[1][1], viewProj[2][1], viewProj[3][1]);
    const glm::vec4 row2(viewProj[0][2], viewProj[1][2], viewProj[2][2], viewProj[3][2]);
    const glm::vec4 row3(viewProj[0][3], viewProj[1][3], viewProj[2][3], viewProj[3][3]);

    glm::vec4 planes[6];
    planes[0] = row3 + row0; // Left
    planes[1] = row3 - row0; // Right
    planes[2] = row3 + row1; // Bottom
    planes[3] = row3 - row1; // Top
    planes[4] = row3 + row2; // Near
    planes[5] = row3 - row2; // Far

    // Normalize planes
    for (int i = 0; i < 6; ++i)
    {
        float length = glm::length(glm::vec3(planes[i]));
        if (length > 0.0f)
        {
            planes[i] /= length;
        }
    }

    // Test each snowflake against frustum
    for (int i = 0; i < m_count; ++i)
    {
        const Snowflake &flake = m_snowflakes[i];
        glm::vec4 flakePos = glm::vec4(flake.position, 1.0f);

        bool visible = true;
        for (int j = 0; j < 6; ++j)
        {
            if (glm::dot(planes[j], flakePos) < 0.0f)
            {
                visible = false;
                break;
            }
        }

        if (visible)
        {
            m_visibleIndices.push_back(i);
            m_visibleCount++;
        }
    }
}

void SnowSystem::initializeBullet()
{
    if (m_bulletWorld)
    {
        return;
    }

    m_bulletCollisionConfig = new btDefaultCollisionConfiguration();
    m_bulletDispatcher = new btCollisionDispatcher(m_bulletCollisionConfig);
    m_bulletBroadphase = new btDbvtBroadphase();
    m_bulletSolver = new btSequentialImpulseConstraintSolver();
    m_bulletWorld = new btDiscreteDynamicsWorld(m_bulletDispatcher, m_bulletBroadphase, m_bulletSolver, m_bulletCollisionConfig);
    m_bulletWorld->setGravity(btVector3(0.0, 0.0, 0.0));

    // Static ground plane at y = m_floorY with upward normal
    m_groundShape = new btStaticPlaneShape(btVector3(0.0, 1.0, 0.0), -m_floorY);
    btTransform groundTransform;
    groundTransform.setIdentity();
    m_groundMotionState = new btDefaultMotionState(groundTransform);
    btRigidBody::btRigidBodyConstructionInfo rbInfo(0.0, m_groundMotionState, m_groundShape);
    m_groundBody = new btRigidBody(rbInfo);
    m_groundBody->setCollisionFlags(m_groundBody->getCollisionFlags() | btCollisionObject::CF_STATIC_OBJECT);
    m_bulletWorld->addRigidBody(m_groundBody);
    std::cout << "[SnowSystem][Bullet] World initialized (static plane at Y = " << m_floorY << ")" << std::endl;
}

void SnowSystem::shutdownBullet()
{
    if (!m_bulletWorld && !m_groundShape)
    {
        return;
    }

    if (m_bulletWorld && m_groundBody)
    {
        m_bulletWorld->removeRigidBody(m_groundBody);
    }
    delete m_groundBody;
    m_groundBody = nullptr;
    delete m_groundMotionState;
    m_groundMotionState = nullptr;
    delete m_groundShape;
    m_groundShape = nullptr;

    delete m_bulletWorld;
    m_bulletWorld = nullptr;
    delete m_bulletSolver;
    m_bulletSolver = nullptr;
    delete m_bulletBroadphase;
    m_bulletBroadphase = nullptr;
    delete m_bulletDispatcher;
    m_bulletDispatcher = nullptr;
    delete m_bulletCollisionConfig;
    m_bulletCollisionConfig = nullptr;
    std::cout << "[SnowSystem][Bullet] World shutdown" << std::endl;
}
