#pragma once
#include "../Registry.h"
#include <SDL3/SDL.h>
#include <glm/gtc/quaternion.hpp>

class PlayerMovementSystem {
public:
    // Player collision radius for cylinder approximation
    static constexpr float PLAYER_RADIUS = 0.4f;

    void update(Registry& registry, float dt) {
        const bool* keys = SDL_GetKeyboardState(nullptr);

        registry.forEachPlayerController([&](Entity entity, Transform& transform, PlayerController& pc) {
            // Get entity's own facing direction
            auto* facing = registry.getFacingDirection(entity);
            if (!facing) return;

            // Forward direction based on facing yaw
            float yawRad = glm::radians(facing->yaw);
            glm::vec3 forward(-sin(yawRad), 0.0f, -cos(yawRad));
            glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

            // Character faces forward direction (model faces +Z by default, so add PI)
            glm::quat targetRot = glm::angleAxis(yawRad + glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
            transform.rotation = glm::slerp(transform.rotation, targetRot, facing->turnSpeed * dt);

            // Movement relative to facing direction
            glm::vec3 moveDir(0.0f);
            bool movingBackward = false;
            bool sprinting = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];

            if (keys[SDL_SCANCODE_W]) moveDir += forward;
            if (keys[SDL_SCANCODE_S]) {
                moveDir -= forward;
                movingBackward = true;
            }
            if (keys[SDL_SCANCODE_A]) moveDir -= right;
            if (keys[SDL_SCANCODE_D]) moveDir += right;

            bool isMoving = glm::length(moveDir) > 0.001f;
            if (isMoving) {
                moveDir = glm::normalize(moveDir);
                float speed = pc.moveSpeed;
                if (movingBackward) {
                    speed *= 0.25f;   // backward: slowest
                } else if (sprinting) {
                    speed *= 1.0f;    // sprinting: full speed
                } else {
                    speed *= 0.5f;    // walking: half speed
                }

                // Calculate desired new position
                glm::vec3 desiredPos = transform.position + moveDir * speed * dt;

                // Check collision with all box colliders and resolve
                glm::vec3 resolvedPos = resolveCollisions(registry, entity, transform.position, desiredPos);
                transform.position = resolvedPos;
            }

            // Play/stop animation based on movement
            auto* anim = registry.getAnimation(entity);
            if (anim) {
                anim->playing = isMoving;

                if (isMoving) {
                    // Animation indices: 0 = running, 1 = walking, 2 = backrun
                    int targetClip;
                    if (movingBackward) {
                        targetClip = 2;  // backrun
                    } else if (sprinting) {
                        targetClip = 0;  // running
                    } else {
                        targetClip = 1;  // walking
                    }

                    if (anim->clipIndex != targetClip) {
                        anim->clipIndex = targetClip;
                        anim->time = 0.0f;  // Reset animation time when switching
                    }
                } else {
                    // Reset to bind pose when stopped
                    auto* skeleton = registry.getSkeleton(entity);
                    if (skeleton) {
                        skeleton->resetToBindPose();
                    }
                }
            }
        });
    }

private:
    // Resolve collisions between player cylinder and box colliders
    // Uses sliding collision response - player slides along walls
    glm::vec3 resolveCollisions(Registry& registry, Entity playerEntity,
                                const glm::vec3& currentPos, const glm::vec3& desiredPos) {
        glm::vec3 newPos = desiredPos;

        // Check against all box colliders
        registry.forEachBoxCollider([&](Entity boxEntity, Transform& boxTransform, BoxCollider& box) {
            // Skip self-collision if player has a collider
            if (boxEntity == playerEntity) return;

            // Skip colliders that are hidden (below ground - building pool optimization)
            if (boxTransform.position.y < -100.0f) return;

            // Get box bounds in world space
            // Note: buildings use scale for dimensions, so halfExtents come from scale
            glm::vec3 boxCenter = boxTransform.position + box.offset;
            glm::vec3 halfExtents = box.halfExtents * boxTransform.scale;

            // For buildings, the box is centered at position with scale as dimensions
            // The unit box mesh goes from Y=0 to Y=1, so center Y needs adjustment
            boxCenter.y += halfExtents.y;  // Adjust center to middle of box

            glm::vec3 boxMin = boxCenter - halfExtents;
            glm::vec3 boxMax = boxCenter + halfExtents;

            // Expand box by player radius for circle-vs-AABB collision
            boxMin.x -= PLAYER_RADIUS;
            boxMin.z -= PLAYER_RADIUS;
            boxMax.x += PLAYER_RADIUS;
            boxMax.z += PLAYER_RADIUS;

            // Check if player (as a point after expansion) is inside expanded box (XZ plane)
            // Only check XZ - we don't want to block vertical movement here
            if (newPos.x > boxMin.x && newPos.x < boxMax.x &&
                newPos.z > boxMin.z && newPos.z < boxMax.z &&
                newPos.y < boxMax.y && newPos.y >= boxMin.y - 1.0f) {  // Height check with small tolerance

                // Calculate penetration on each axis
                float penLeft = newPos.x - boxMin.x;
                float penRight = boxMax.x - newPos.x;
                float penBack = newPos.z - boxMin.z;
                float penFront = boxMax.z - newPos.z;

                // Find minimum penetration axis for sliding response
                float minPenX = std::min(penLeft, penRight);
                float minPenZ = std::min(penBack, penFront);

                // Push out on the axis with least penetration (sliding behavior)
                if (minPenX < minPenZ) {
                    // Push out on X axis
                    if (penLeft < penRight) {
                        newPos.x = boxMin.x;  // Push left
                    } else {
                        newPos.x = boxMax.x;  // Push right
                    }
                } else {
                    // Push out on Z axis
                    if (penBack < penFront) {
                        newPos.z = boxMin.z;  // Push back
                    } else {
                        newPos.z = boxMax.z;  // Push front
                    }
                }
            }
        });

        return newPos;
    }
};
