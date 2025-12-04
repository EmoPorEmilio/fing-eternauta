#pragma once

#include <glm/glm.hpp>

// Forward declarations for Bullet3
class btRigidBody;
class btCollisionShape;

namespace ecs {

// Physics body type
enum class PhysicsBodyType : uint8_t {
    STATIC,     // Doesn't move, infinite mass
    DYNAMIC,    // Fully simulated
    KINEMATIC   // Moved by code, affects dynamic bodies
};

struct PhysicsComponent {
    PhysicsBodyType bodyType = PhysicsBodyType::DYNAMIC;

    // Bullet3 handles (optional, for Bullet integration)
    btRigidBody* rigidBody = nullptr;
    btCollisionShape* collisionShape = nullptr;

    // Basic physics properties (used if not using Bullet)
    glm::vec3 velocity{0.0f};
    glm::vec3 angularVelocity{0.0f};
    glm::vec3 acceleration{0.0f};

    // Material properties
    float mass = 1.0f;
    float friction = 0.5f;
    float restitution = 0.0f;  // Bounciness
    float linearDamping = 0.0f;
    float angularDamping = 0.05f;

    // Collision flags
    bool useBullet = false;    // Use Bullet3 physics
    bool useGravity = true;
    bool isTrigger = false;    // Collision detection only, no response

    // Collision layers (bitmask)
    uint32_t collisionLayer = 1;
    uint32_t collisionMask = 0xFFFFFFFF;

    PhysicsComponent() = default;

    // Static body (doesn't move)
    static PhysicsComponent Static() {
        PhysicsComponent p;
        p.bodyType = PhysicsBodyType::STATIC;
        p.mass = 0.0f;
        p.useGravity = false;
        return p;
    }

    // Dynamic body with mass
    static PhysicsComponent Dynamic(float mass = 1.0f) {
        PhysicsComponent p;
        p.bodyType = PhysicsBodyType::DYNAMIC;
        p.mass = mass;
        return p;
    }

    // Kinematic body (moved by code)
    static PhysicsComponent Kinematic() {
        PhysicsComponent p;
        p.bodyType = PhysicsBodyType::KINEMATIC;
        p.mass = 0.0f;
        p.useGravity = false;
        return p;
    }
};

} // namespace ecs
