#pragma once
#include "../Registry.h"
#include <glm/glm.hpp>

class CameraOrbitSystem {
public:
    void update(Registry& registry, int mouseX, int mouseY) {
        registry.forEachFollowTarget([&](Entity camEntity, Transform& transform, FollowTarget& ft) {
            if (ft.target == NULL_ENTITY) return;

            // Get target's facing direction - mouse controls player's facing, not camera
            auto* facing = registry.getFacingDirection(ft.target);
            if (!facing) return;

            facing->yaw -= mouseX * ft.sensitivity;
            ft.pitch -= mouseY * ft.sensitivity;
            ft.pitch = glm::clamp(ft.pitch, -60.0f, 80.0f);
        });
    }
};
