#pragma once
#include <glm/glm.hpp>

struct RigidBody {
    glm::vec3 velocity{0.0f};
    float gravity = 9.8f;
    bool grounded = false;
};
