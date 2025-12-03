#pragma once

#include "../ecs/System.h"
#include "../components/TransformComponent.h"
#include "../components/ParticleComponent.h"
#include "../components/RenderableComponent.h"
#include <glm/glm.hpp>
#include <random>

namespace ecs {

// Handles particle simulation (snow, etc.)
class ParticleSystem : public System {
public:
    ParticleSystem() : m_rng(std::random_device{}()) {}

    void update(Registry& registry, float deltaTime) override {
        // Update all particles
        registry.each<TransformComponent, ParticleComponent>(
            [this, deltaTime](Entity entity, TransformComponent& transform, ParticleComponent& particle) {
                if (!particle.alive) return;

                // Store previous position for motion blur
                particle.prevPosition = transform.position;

                // Handle settled snow
                if (particle.type == ParticleType::SNOW && particle.settled) {
                    particle.settleTimer -= deltaTime;
                    if (particle.settleTimer <= 0.0f) {
                        // Respawn
                        respawnSnowflake(transform, particle);
                    }
                    return;
                }

                // Apply wind (snow-specific)
                if (particle.type == ParticleType::SNOW) {
                    float windEffect = sin(m_time * 2.0f + particle.seed * 10.0f) * m_windSpeed;
                    particle.velocity.x = m_windDirection.x * windEffect;
                    particle.velocity.z = m_windDirection.z * windEffect;
                    particle.velocity.y = -particle.fallSpeed;

                    // Swirl effect
                    float swirlX = sin(m_time * 3.0f + particle.seed * 5.0f) * 0.5f;
                    float swirlZ = cos(m_time * 3.0f + particle.seed * 5.0f) * 0.5f;
                    particle.velocity.x += swirlX;
                    particle.velocity.z += swirlZ;
                }

                // Apply velocity
                transform.position += particle.velocity * deltaTime;
                transform.dirty = true;

                // Ground collision (simple)
                if (transform.position.y <= m_groundLevel) {
                    if (particle.type == ParticleType::SNOW) {
                        // Settle on ground
                        particle.settled = true;
                        particle.settleTimer = m_settleTime;
                        transform.position.y = m_groundLevel;
                    } else {
                        // Respawn other particle types
                        particle.alive = false;
                    }
                }

                // Age update
                particle.updateAge(deltaTime);
            });

        m_time += deltaTime;
    }

    const char* name() const override { return "ParticleSystem"; }

    // Wind settings
    void setWindSpeed(float speed) { m_windSpeed = speed; }
    void setWindDirection(float yawDegrees) {
        float rad = glm::radians(yawDegrees);
        m_windDirection = glm::vec3(cos(rad), 0.0f, sin(rad));
    }

    // Ground settings
    void setGroundLevel(float level) { m_groundLevel = level; }
    void setSettleTime(float time) { m_settleTime = time; }

    // Spawn area
    void setSpawnHeight(float height) { m_spawnHeight = height; }
    void setSpawnRadius(float radius) { m_spawnRadius = radius; }

    // Get settings for UI
    float getWindSpeed() const { return m_windSpeed; }
    float getSpawnHeight() const { return m_spawnHeight; }
    float getSpawnRadius() const { return m_spawnRadius; }

private:
    std::mt19937 m_rng;
    float m_time = 0.0f;

    // Wind
    float m_windSpeed = 2.0f;
    glm::vec3 m_windDirection{1.0f, 0.0f, 0.0f};

    // Ground
    float m_groundLevel = 0.0f;
    float m_settleTime = 2.0f;

    // Spawn area
    float m_spawnHeight = 50.0f;
    float m_spawnRadius = 100.0f;
    glm::vec3 m_spawnCenter{0.0f};

    void respawnSnowflake(TransformComponent& transform, ParticleComponent& particle) {
        std::uniform_real_distribution<float> distXZ(-m_spawnRadius, m_spawnRadius);
        std::uniform_real_distribution<float> distSpeed(0.8f, 1.2f);
        std::uniform_real_distribution<float> distSeed(0.0f, 1.0f);

        transform.position.x = m_spawnCenter.x + distXZ(m_rng);
        transform.position.y = m_spawnHeight;
        transform.position.z = m_spawnCenter.z + distXZ(m_rng);
        transform.dirty = true;

        particle.prevPosition = transform.position;
        particle.fallSpeed = particle.fallSpeed * distSpeed(m_rng);
        particle.seed = distSeed(m_rng);
        particle.settled = false;
        particle.settleTimer = 0.0f;
        particle.alive = true;
        particle.age = 0.0f;
    }

public:
    void setSpawnCenter(const glm::vec3& center) { m_spawnCenter = center; }
};

} // namespace ecs
