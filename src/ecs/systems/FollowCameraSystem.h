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

            camTransform.position = getCameraPosition(targetTransform->position, ft, facing->yaw);
        });
    }

    // Calculate camera position for a given target position, follow settings, and yaw
    // This is the single source of truth for camera positioning
    static glm::vec3 getCameraPosition(const glm::vec3& targetPos, const FollowTarget& ft, float yaw) {
        float yawRad = glm::radians(yaw);
        float pitchRad = glm::radians(ft.pitch);

        glm::vec3 forward(-sin(yawRad), 0.0f, -cos(yawRad));
        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));

        // Camera orbit position based on pitch
        float verticalOffset = ft.height - sin(pitchRad) * ft.distance * 0.5f;
        float horizontalDistance = ft.distance * cos(pitchRad * 0.5f);

        // Camera positioned behind and to the right (over shoulder)
        glm::vec3 camPos = targetPos - forward * horizontalDistance + right * ft.shoulderOffset;
        camPos.y += verticalOffset;

        return camPos;
    }

    // Get look-at position for render system
    static glm::vec3 getLookAtPosition(const Transform& targetTransform, const FollowTarget& ft, float yaw) {
        float yawRad = glm::radians(yaw);
        glm::vec3 forward(-sin(yawRad), 0.0f, -cos(yawRad));

        // Look at point ahead of character at fixed height (character stays upright)
        glm::vec3 lookAt = targetTransform.position + forward * ft.lookAhead;
        lookAt.y += 1.0f;
        return lookAt;
    }
};
