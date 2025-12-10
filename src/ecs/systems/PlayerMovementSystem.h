#pragma once
#include "../Registry.h"
#include "../../culling/BuildingCuller.h"
#include "../../culling/Octree.h"
#include "../../procedural/BuildingGenerator.h"
#include <SDL3/SDL.h>
#include <glm/gtc/quaternion.hpp>

class PlayerMovementSystem {
public:
    // Player collision radius for cylinder approximation
    static constexpr float PLAYER_RADIUS = 0.4f;
    // Query radius for nearby buildings - needs to be large enough to catch buildings
    // when player is in street (buildings are 8 wide, streets are 12 wide, block = 20)
    static constexpr float COLLISION_QUERY_RADIUS = 15.0f;

    void update(Registry& registry, float dt, const BuildingCuller* buildingCuller = nullptr, const AABB* extraAABB = nullptr) {
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

                // Check collision with buildings and resolve
                glm::vec3 resolvedPos = resolveCollisions(registry, entity, transform.position, desiredPos, buildingCuller, extraAABB);
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
    // Resolve collisions between player cylinder and buildings
    // Uses sliding collision response - player slides along walls
    glm::vec3 resolveCollisions(Registry& registry, Entity playerEntity,
                                const glm::vec3& currentPos, const glm::vec3& desiredPos,
                                const BuildingCuller* buildingCuller, const AABB* extraAABB) {
        glm::vec3 newPos = desiredPos;

        // Helper lambda to check collision against a single AABB
        auto checkAABB = [&](const AABB& aabb) {
            // Expand box by player radius for circle-vs-AABB collision
            AABB expanded = aabb;
            expanded.min.x -= PLAYER_RADIUS;
            expanded.min.z -= PLAYER_RADIUS;
            expanded.max.x += PLAYER_RADIUS;
            expanded.max.z += PLAYER_RADIUS;

            // Check if player is inside expanded box (XZ plane)
            if (newPos.x > expanded.min.x && newPos.x < expanded.max.x &&
                newPos.z > expanded.min.z && newPos.z < expanded.max.z &&
                newPos.y < expanded.max.y && newPos.y >= expanded.min.y - 1.0f) {

                // Calculate penetration on each axis
                float penLeft = newPos.x - expanded.min.x;
                float penRight = expanded.max.x - newPos.x;
                float penBack = newPos.z - expanded.min.z;
                float penFront = expanded.max.z - newPos.z;

                // Find minimum penetration axis for sliding response
                float minPenX = std::min(penLeft, penRight);
                float minPenZ = std::min(penBack, penFront);

                // Push out on the axis with least penetration (sliding behavior)
                if (minPenX < minPenZ) {
                    if (penLeft < penRight) {
                        newPos.x = expanded.min.x;
                    } else {
                        newPos.x = expanded.max.x;
                    }
                } else {
                    if (penBack < penFront) {
                        newPos.z = expanded.min.z;
                    } else {
                        newPos.z = expanded.max.z;
                    }
                }
            }
        };

        // Check against buildings in octree
        if (buildingCuller) {
            buildingCuller->queryRadius(newPos, COLLISION_QUERY_RADIUS,
                [&](const BuildingGenerator::BuildingData& building) {
                    // Build AABB for this building
                    glm::vec3 halfExtents(building.width * 0.5f, building.height * 0.5f, building.depth * 0.5f);
                    glm::vec3 center = building.position + glm::vec3(0.0f, building.height * 0.5f, 0.0f);
                    AABB buildingAABB = AABB::fromCenterExtents(center, halfExtents);
                    checkAABB(buildingAABB);
                });
        }

        // Check against extra AABB (e.g., FING building)
        if (extraAABB) {
            checkAABB(*extraAABB);
        }

        // Also check against any remaining box colliders in registry (non-building objects)
        registry.forEachBoxCollider([&](Entity boxEntity, Transform& boxTransform, BoxCollider& box) {
            if (boxEntity == playerEntity) return;
            if (boxTransform.position.y < -100.0f) return;

            glm::vec3 boxCenter = boxTransform.position + box.offset;
            glm::vec3 halfExtents = box.halfExtents * boxTransform.scale;
            boxCenter.y += halfExtents.y;

            AABB boxAABB;
            boxAABB.min = boxCenter - halfExtents;
            boxAABB.max = boxCenter + halfExtents;
            checkAABB(boxAABB);
        });

        return newPos;
    }
};
