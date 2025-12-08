#pragma once
#include "../Registry.h"
#include <glm/glm.hpp>

class FollowCameraSystem {
public:
    void update(Registry& registry) {
        registry.forEachFollowTarget([&](Entity camEntity, Transform& camTransform, FollowTarget& ft) {
            if (ft.target == NULL_ENTITY) return;

            auto* targetTransform = registry.getTransform(ft.target);
            auto* facing = registry.getFacingDirection(ft.target);
            if (!targetTransform || !facing) return;

            // Forward and right directions based on target's facing yaw
            float yawRad = glm::radians(facing->yaw);
            glm::vec3 forward(-sin(yawRad), 0.0f, -cos(yawRad));
            glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

            // Camera positioned behind and to the right (over shoulder)
            glm::vec3 camPos = targetTransform->position - forward * ft.distance + right * ft.shoulderOffset;
            camPos.y += ft.height;

            camTransform.position = camPos;
        });
    }

    // Get look-at position for render system (now needs facing direction)
    static glm::vec3 getLookAtPosition(const Transform& targetTransform, const FollowTarget& ft, float yaw) {
        float yawRad = glm::radians(yaw);
        glm::vec3 forward(-sin(yawRad), 0.0f, -cos(yawRad));

        // Look at point ahead of character, slightly down based on pitch
        glm::vec3 lookAt = targetTransform.position + forward * ft.lookAhead;
        lookAt.y += 1.0f - tan(glm::radians(ft.pitch)) * ft.lookAhead * 0.1f;
        return lookAt;
    }
};
