#pragma once
#include "../Registry.h"

class PhysicsSystem {
public:
    void update(Registry& registry, float dt) {
        registry.forEachRigidBody([&](Entity entity, Transform& transform, RigidBody& rb) {
            if (rb.grounded) return;

            // Apply gravity
            rb.velocity.y -= rb.gravity * dt;

            // Update position
            transform.position += rb.velocity * dt;
        });
    }
};
