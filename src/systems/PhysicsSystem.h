#pragma once

#include "../ecs/System.h"
#include "../components/TransformComponent.h"
#include "../components/PhysicsComponent.h"
#include <glm/glm.hpp>

// Forward declarations for Bullet3
class btDiscreteDynamicsWorld;
class btDefaultCollisionConfiguration;
class btCollisionDispatcher;
class btBroadphaseInterface;
class btSequentialImpulseConstraintSolver;

namespace ecs {

// Handles physics simulation (simple integration or Bullet3)
class PhysicsSystem : public System {
public:
    PhysicsSystem() = default;
    ~PhysicsSystem() {
        cleanupBullet();
    }

    void init(Registry& registry) override {
        (void)registry;
        // Bullet world can be initialized here if needed
    }

    void update(Registry& registry, float deltaTime) override {
        // Step Bullet world if initialized
        if (m_bulletWorld && m_useBullet) {
            stepBulletWorld(deltaTime);
            syncBulletToECS(registry);
        }

        // Simple physics integration for non-Bullet entities
        registry.each<TransformComponent, PhysicsComponent>(
            [deltaTime](TransformComponent& transform, PhysicsComponent& physics) {
                // Skip if using Bullet (handled by syncBulletToECS)
                if (physics.useBullet && physics.rigidBody) return;

                // Skip static bodies
                if (physics.bodyType == PhysicsBodyType::STATIC) return;

                // Apply gravity
                if (physics.useGravity) {
                    physics.velocity.y -= 9.81f * deltaTime;
                }

                // Apply acceleration
                physics.velocity += physics.acceleration * deltaTime;

                // Apply damping
                physics.velocity *= (1.0f - physics.linearDamping * deltaTime);

                // Update position
                transform.position += physics.velocity * deltaTime;
                transform.dirty = true;
            });
    }

    const char* name() const override { return "PhysicsSystem"; }

    // Gravity
    void setGravity(const glm::vec3& gravity) { m_gravity = gravity; }
    glm::vec3 getGravity() const { return m_gravity; }

    // Bullet3 integration
    void setUseBullet(bool use) { m_useBullet = use; }
    bool isUsingBullet() const { return m_useBullet; }

    // Initialize Bullet world (call from outside with proper includes)
    void setBulletWorld(btDiscreteDynamicsWorld* world) {
        m_bulletWorld = world;
    }

    btDiscreteDynamicsWorld* getBulletWorld() const {
        return m_bulletWorld;
    }

private:
    glm::vec3 m_gravity{0.0f, -9.81f, 0.0f};
    bool m_useBullet = false;

    // Bullet world (owned externally, we just reference it)
    btDiscreteDynamicsWorld* m_bulletWorld = nullptr;

    void stepBulletWorld(float deltaTime) {
        if (m_bulletWorld) {
            m_bulletWorld->stepSimulation(deltaTime, 10);
        }
    }

    void syncBulletToECS(Registry& registry) {
        registry.each<TransformComponent, PhysicsComponent>(
            [](TransformComponent& transform, PhysicsComponent& physics) {
                if (!physics.useBullet || !physics.rigidBody) return;

                // Get transform from Bullet rigid body
                // Note: Actual implementation would use btTransform
                // This is a placeholder - full implementation needs Bullet headers
            });
    }

    void cleanupBullet() {
        // World is owned externally, don't delete here
        m_bulletWorld = nullptr;
    }
};

} // namespace ecs
