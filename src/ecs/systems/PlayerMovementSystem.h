#pragma once
#include "../Registry.h"
#include <SDL.h>
#include <glm/gtc/quaternion.hpp>

class PlayerMovementSystem {
public:
    void update(Registry& registry, float dt, float cameraYaw) {
        const Uint8* keys = SDL_GetKeyboardState(nullptr);

        // Forward direction based on camera yaw
        float yawRad = glm::radians(cameraYaw);
        glm::vec3 forward(-sin(yawRad), 0.0f, -cos(yawRad));
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

        registry.forEachPlayerController([&](Entity entity, Transform& transform, PlayerController& pc) {
            // Character faces forward direction (model faces +Z by default, so add PI)
            glm::quat targetRot = glm::angleAxis(yawRad + glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
            transform.rotation = glm::slerp(transform.rotation, targetRot, pc.turnSpeed * dt);

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
                transform.position += moveDir * speed * dt;
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
};
