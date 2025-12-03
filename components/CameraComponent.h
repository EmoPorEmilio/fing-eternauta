#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ecs {

// Camera projection type
enum class ProjectionType : uint8_t {
    PERSPECTIVE,
    ORTHOGRAPHIC
};

struct CameraComponent {
    ProjectionType projection = ProjectionType::PERSPECTIVE;

    // View parameters (FPS-style)
    float yaw = -90.0f;         // Horizontal rotation (degrees)
    float pitch = 0.0f;         // Vertical rotation (degrees)
    float roll = 0.0f;          // Not typically used in FPS

    // Cached direction vectors (updated by CameraSystem)
    glm::vec3 front{0.0f, 0.0f, -1.0f};
    glm::vec3 up{0.0f, 1.0f, 0.0f};
    glm::vec3 right{1.0f, 0.0f, 0.0f};
    glm::vec3 worldUp{0.0f, 1.0f, 0.0f};

    // Perspective parameters
    float fov = 45.0f;          // Field of view (degrees)
    float aspectRatio = 16.0f / 9.0f;
    float nearPlane = 0.1f;
    float farPlane = 1000.0f;

    // Orthographic parameters
    float orthoSize = 10.0f;    // Half-height of the view

    // Movement settings
    float movementSpeed = 5.0f;
    float fastMovementSpeed = 15.0f;
    float mouseSensitivity = 0.1f;

    // Constraints
    float minPitch = -89.0f;
    float maxPitch = 89.0f;

    // State
    bool isActive = true;       // Is this the active camera?
    bool constrainPitch = true;

    // Cached matrices (updated by CameraSystem)
    glm::mat4 viewMatrix{1.0f};
    glm::mat4 projectionMatrix{1.0f};
    bool matricesDirty = true;

    CameraComponent() = default;

    // Update direction vectors from yaw/pitch
    void updateVectors() {
        glm::vec3 newFront;
        newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        newFront.y = sin(glm::radians(pitch));
        newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(newFront);

        right = glm::normalize(glm::cross(front, worldUp));
        up = glm::normalize(glm::cross(right, front));

        matricesDirty = true;
    }

    // Apply mouse input
    void applyMouseInput(float xOffset, float yOffset) {
        yaw += xOffset * mouseSensitivity;
        pitch += yOffset * mouseSensitivity;

        if (constrainPitch) {
            if (pitch > maxPitch) pitch = maxPitch;
            if (pitch < minPitch) pitch = minPitch;
        }

        updateVectors();
    }

    // Get view matrix (uses position from TransformComponent)
    glm::mat4 calculateViewMatrix(const glm::vec3& position) const {
        return glm::lookAt(position, position + front, up);
    }

    // Get projection matrix
    glm::mat4 calculateProjectionMatrix() const {
        if (projection == ProjectionType::PERSPECTIVE) {
            return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
        } else {
            float halfWidth = orthoSize * aspectRatio;
            float halfHeight = orthoSize;
            return glm::ortho(-halfWidth, halfWidth, -halfHeight, halfHeight, nearPlane, farPlane);
        }
    }
};

} // namespace ecs
