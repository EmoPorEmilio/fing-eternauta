#include "FollowCameraSystem.h"
#include "../../culling/BuildingCuller.h"

glm::vec3 FollowCameraSystem::resolveCollision(const glm::vec3& lookAt, const glm::vec3& desiredCamPos,
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
