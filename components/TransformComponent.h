#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ecs {

struct TransformComponent {
    glm::vec3 position{0.0f};
    glm::quat rotation{1.0f, 0.0f, 0.0f, 0.0f}; // Identity quaternion
    glm::vec3 scale{1.0f};

    // Cached model matrix (updated by TransformSystem)
    glm::mat4 modelMatrix{1.0f};

    // Dirty flag - set true when position/rotation/scale change
    bool dirty = true;

    TransformComponent() = default;

    explicit TransformComponent(const glm::vec3& pos)
        : position(pos), dirty(true) {}

    TransformComponent(const glm::vec3& pos, const glm::quat& rot)
        : position(pos), rotation(rot), dirty(true) {}

    TransformComponent(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scl)
        : position(pos), rotation(rot), scale(scl), dirty(true) {}

    // Convenience: set position and mark dirty
    void setPosition(const glm::vec3& pos) {
        position = pos;
        dirty = true;
    }

    void setRotation(const glm::quat& rot) {
        rotation = rot;
        dirty = true;
    }

    void setScale(const glm::vec3& scl) {
        scale = scl;
        dirty = true;
    }

    // Update model matrix from position/rotation/scale
    void updateModelMatrix() {
        if (dirty) {
            modelMatrix = glm::translate(glm::mat4(1.0f), position);
            modelMatrix *= glm::mat4_cast(rotation);
            modelMatrix = glm::scale(modelMatrix, scale);
            dirty = false;
        }
    }

    // Get forward direction
    glm::vec3 forward() const {
        return rotation * glm::vec3(0.0f, 0.0f, -1.0f);
    }

    // Get right direction
    glm::vec3 right() const {
        return rotation * glm::vec3(1.0f, 0.0f, 0.0f);
    }

    // Get up direction
    glm::vec3 up() const {
        return rotation * glm::vec3(0.0f, 1.0f, 0.0f);
    }
};

} // namespace ecs
