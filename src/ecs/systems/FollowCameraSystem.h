#pragma once
#include "../Registry.h"
#include "../../culling/BuildingCuller.h"
#include <glm/glm.hpp>

class FollowCameraSystem {
public:
    // Camera collision offset - how far from wall to place camera
    static constexpr float COLLISION_OFFSET = 0.5f;

    void update(Registry& registry) {
        registry.forEachFollowTarget([&](Entity camEntity, Transform& camTransform, FollowTarget& ft) {
            if (ft.target == NULL_ENTITY) return;

            auto* targetTransform = registry.getTransform(ft.target);
            auto* facing = registry.getFacingDirection(ft.target);
            if (!targetTransform || !facing) return;

            camTransform.position = getCameraPosition(targetTransform->position, ft, facing->yaw);
        });
    }

    // Update with camera collision against buildings
    void updateWithCollision(Registry& registry, const BuildingCuller& culler, const AABB* extraAABB = nullptr) {
        registry.forEachFollowTarget([&](Entity camEntity, Transform& camTransform, FollowTarget& ft) {
            if (ft.target == NULL_ENTITY) return;

            auto* targetTransform = registry.getTransform(ft.target);
            auto* facing = registry.getFacingDirection(ft.target);
            if (!targetTransform || !facing) return;

            glm::vec3 desiredPos = getCameraPosition(targetTransform->position, ft, facing->yaw);

            // For collision, cast from character position (at shoulder height) toward camera
            // This detects walls between character and camera
            glm::vec3 characterPos = targetTransform->position;
            characterPos.y += 1.5f;  // Shoulder height

            camTransform.position = resolveCollision(characterPos, desiredPos, culler, extraAABB);
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

    // Resolve camera collision - move camera closer if obstructed
    static glm::vec3 resolveCollision(const glm::vec3& lookAt, const glm::vec3& desiredCamPos,
                                       const BuildingCuller& culler, const AABB* extraAABB) {
        glm::vec3 toCamera = desiredCamPos - lookAt;
        float desiredDist = glm::length(toCamera);

        if (desiredDist < 0.01f) {
            return desiredCamPos;  // Camera essentially at look-at point
        }

        glm::vec3 direction = toCamera / desiredDist;

        float hitDist;
        if (culler.raycastWithExtra(lookAt, direction, desiredDist, extraAABB, hitDist)) {
            // Hit something - move camera to hit point minus offset
            float newDist = glm::max(hitDist - COLLISION_OFFSET, 0.1f);  // Don't go behind look-at point
            return lookAt + direction * newDist;
        }

        return desiredCamPos;
    }
};
