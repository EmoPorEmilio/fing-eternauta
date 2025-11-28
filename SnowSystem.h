#pragma once

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <vector>
#include <random>
#include <cstddef>

class Shader;

// Forward declarations for Bullet types to avoid heavy includes in header
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;
class btDiscreteDynamicsWorld;
class btCollisionShape;
class btDefaultMotionState;
class btRigidBody;

struct Snowflake
{
    glm::vec3 position;
    glm::vec3 prevPosition;   // For future motion blur
    float seed;               // Random seed for per-flake variation
    float fallSpeed;          // Individual fall speed variation
    bool settled = false;     // Settled on ground briefly
    float settleTimer = 0.0f; // Seconds remaining while settled
};

class SnowSystem
{
public:
    SnowSystem();
    ~SnowSystem();

    bool initialize();
    void shutdown();

    void update(float deltaTime, const glm::vec3 &cameraPos, const glm::mat4 &viewMatrix, const glm::mat4 &projectionMatrix);
    void render(const glm::mat4 &view, const glm::mat4 &projection, const glm::vec3 &cameraPos);

    // Configuration
    void setEnabled(bool enabled) { m_enabled = enabled; }
    void setCount(int count);
    void setFallSpeed(float speed) { m_fallSpeed = speed; }
    void setWindSpeed(float speed) { m_windSpeed = speed; }
    void setWindDirection(float yawDegrees) { m_windDirection = glm::radians(yawDegrees); }
    void setSpriteSize(float size) { m_spriteSize = size; }
    void setTimeScale(float scale) { m_timeScale = scale; }

    // Bullet physics (minimal ground collision)
    void setBulletGroundCollisionEnabled(bool enabled);

    // Performance settings
    void setFrustumCulling(bool enabled) { m_frustumCulling = enabled; }
    bool isFrustumCullingEnabled() const { return m_frustumCulling; }

    // Fog settings
    void setFogEnabled(bool enabled) { m_fogEnabled = enabled; }
    void setFogColor(const glm::vec3 &color) { m_fogColor = color; }
    void setFogDensity(float density) { m_fogDensity = density; }
    void setFogDesaturationStrength(float strength) { m_fogDesaturationStrength = strength; }
    void setFogAbsorption(float density, float strength)
    {
        m_fogAbsorptionDensity = density;
        m_fogAbsorptionStrength = strength;
    }

    // Getters for UI
    bool isEnabled() const { return m_enabled; }
    int getCount() const { return m_count; }
    float getFallSpeed() const { return m_fallSpeed; }
    float getWindSpeed() const { return m_windSpeed; }
    float getWindDirection() const { return glm::degrees(m_windDirection); }
    float getSpriteSize() const { return m_spriteSize; }
    float getTimeScale() const { return m_timeScale; }

private:
    void setupBuffers();
    void updateBuffers();
    void updatePuffs(float deltaTime);
    void uploadPuffs();
    void respawnFlake(int index);
    glm::vec3 getRandomSpawnPosition(const glm::vec3 &cameraPos, const glm::mat4 &viewMatrix);
    void performFrustumCulling(const glm::mat4 &viewProj);

    // Bullet helpers
    void initializeBullet();
    void shutdownBullet();

    // State
    bool m_enabled;
    bool m_initialized;

    // Snowflake data
    std::vector<Snowflake> m_snowflakes;
    int m_count;

    // Physics parameters
    float m_fallSpeed;
    float m_windSpeed;
    float m_windDirection; // Radians
    float m_spriteSize;
    float m_timeScale;
    float m_accumulatedTime;

    // Spawn bounds
    float m_spawnHeight;
    float m_spawnRadius;
    float m_floorY;

    // Performance settings
    bool m_frustumCulling;
    int m_visibleCount;
    int m_drawCount;
    std::vector<int> m_visibleIndices;
    glm::mat4 m_lastViewProj;

    // Fog settings
    bool m_fogEnabled;
    glm::vec3 m_fogColor;
    float m_fogDensity;
    float m_fogDesaturationStrength;
    float m_fogAbsorptionDensity;
    float m_fogAbsorptionStrength;

    // OpenGL resources
    GLuint m_quadVAO;
    GLuint m_quadVBO;
    GLuint m_instanceVBO;
    GLuint m_puffVAO;
    GLuint m_puffInstanceVBO;
    Shader *m_shader;

    // Random number generation
    std::mt19937 m_rng;
    std::uniform_real_distribution<float> m_uniformDist;

    // Bullet3 minimal world (static ground plane + ray tests)
    bool m_bulletEnabled;
    btDefaultCollisionConfiguration *m_bulletCollisionConfig;
    btCollisionDispatcher *m_bulletDispatcher;
    btBroadphaseInterface *m_bulletBroadphase;
    btSequentialImpulseConstraintSolver *m_bulletSolver;
    btDiscreteDynamicsWorld *m_bulletWorld;
    btCollisionShape *m_groundShape;
    btDefaultMotionState *m_groundMotionState;
    btRigidBody *m_groundBody;

    // Impact puffs (simple billboard discs that fade out)
    struct ImpactPuff
    {
        glm::vec3 position;
        float age;
        float lifetime;
    };
    std::vector<ImpactPuff> m_puffs;
    float m_settleDuration = 0.35f; // seconds a flake rests on ground
    float m_puffLifetime = 0.45f;   // seconds puff fades out
    float m_puffSize = 0.12f;       // world-space half-size of puff disc
};
