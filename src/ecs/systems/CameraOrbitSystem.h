#pragma once
#include "../Registry.h"
#include <glm/glm.hpp>

class CameraOrbitSystem {
public:
    void update(Registry& registry, int mouseX, int mouseY) {
        registry.forEachFollowTarget([&](Entity entity, Transform& transform, FollowTarget& ft) {
            ft.yaw -= mouseX * ft.sensitivity;
            ft.pitch -= mouseY * ft.sensitivity;
            ft.pitch = glm::clamp(ft.pitch, -60.0f, 80.0f);
        });
    }
};
