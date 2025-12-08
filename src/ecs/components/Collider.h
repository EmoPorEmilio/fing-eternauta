#pragma once
#include <glm/glm.hpp>

struct GroundPlane {
    float height = 0.0f;
};

struct BoxCollider {
    glm::vec3 halfExtents{0.5f};  // Half-size in each direction
    glm::vec3 offset{0.0f};       // Offset from entity position
};
