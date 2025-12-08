#pragma once
#include "../Registry.h"

class CollisionSystem {
public:
    void update(Registry& registry) {
        // Check each rigid body against all box colliders
        registry.forEachRigidBody([&](Entity rbEntity, Transform& rbTransform, RigidBody& rb) {
            if (rb.grounded) return;

            glm::vec3 pos = rbTransform.position;

            registry.forEachBoxCollider([&](Entity boxEntity, Transform& boxTransform, BoxCollider& box) {
                glm::vec3 boxPos = boxTransform.position + box.offset;
                glm::vec3 boxMin = boxPos - box.halfExtents;
                glm::vec3 boxMax = boxPos + box.halfExtents;

                // Check if rigid body is within box XZ bounds
                if (pos.x >= boxMin.x && pos.x <= boxMax.x &&
                    pos.z >= boxMin.z && pos.z <= boxMax.z) {

                    // Check if falling onto the top of the box
                    if (pos.y <= boxMax.y && rb.velocity.y <= 0.0f) {
                        rbTransform.position.y = boxMax.y;
                        rb.velocity = glm::vec3(0.0f);
                        rb.grounded = true;
                    }
                }
            });
        });
    }
};
