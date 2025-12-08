#pragma once
#include "../Registry.h"
#include <SDL3/SDL.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

class FreeCameraSystem {
public:
    void update(Registry& registry, float dt, int mouseDeltaX, int mouseDeltaY) {
        const bool* keys = SDL_GetKeyboardState(nullptr);

        Entity camEntity = registry.getActiveCamera();
        if (camEntity == NULL_ENTITY) return;

        auto* camTransform = registry.getTransform(camEntity);
        if (!camTransform) return;

        // Mouse look
        m_yaw -= mouseDeltaX * m_mouseSensitivity;
        m_pitch -= mouseDeltaY * m_mouseSensitivity;
        m_pitch = glm::clamp(m_pitch, -89.0f, 89.0f);

        // Calculate forward/right vectors from yaw/pitch
        float yawRad = glm::radians(m_yaw);
        float pitchRad = glm::radians(m_pitch);

        glm::vec3 forward;
        forward.x = cos(pitchRad) * sin(yawRad);
        forward.y = sin(pitchRad);
        forward.z = cos(pitchRad) * cos(yawRad);
        forward = glm::normalize(forward);

        glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0.0f, 1.0f, 0.0f)));
        glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

        // WASD movement
        float speed = m_moveSpeed;
        if (keys[SDL_SCANCODE_LSHIFT]) {
            speed *= 3.0f;
        }

        glm::vec3 velocity(0.0f);
        if (keys[SDL_SCANCODE_W]) velocity += forward;
        if (keys[SDL_SCANCODE_S]) velocity -= forward;
        if (keys[SDL_SCANCODE_A]) velocity -= right;
        if (keys[SDL_SCANCODE_D]) velocity += right;
        if (keys[SDL_SCANCODE_E] || keys[SDL_SCANCODE_SPACE]) velocity += up;
        if (keys[SDL_SCANCODE_Q] || keys[SDL_SCANCODE_LCTRL]) velocity -= up;

        if (glm::length(velocity) > 0.0f) {
            velocity = glm::normalize(velocity) * speed * dt;
            camTransform->position += velocity;
        }

        // Store forward for view matrix calculation
        m_forward = forward;
    }

    glm::mat4 getViewMatrix(const glm::vec3& position) const {
        return glm::lookAt(position, position + m_forward, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    void setPosition(const glm::vec3& pos, float yaw, float pitch) {
        m_yaw = yaw;
        m_pitch = pitch;

        float yawRad = glm::radians(m_yaw);
        float pitchRad = glm::radians(m_pitch);
        m_forward.x = cos(pitchRad) * sin(yawRad);
        m_forward.y = sin(pitchRad);
        m_forward.z = cos(pitchRad) * cos(yawRad);
        m_forward = glm::normalize(m_forward);
    }

    float yaw() const { return m_yaw; }
    float pitch() const { return m_pitch; }
    glm::vec3 forward() const { return m_forward; }

private:
    float m_yaw = 0.0f;
    float m_pitch = 0.0f;
    float m_mouseSensitivity = 0.15f;
    float m_moveSpeed = 5.0f;
    glm::vec3 m_forward = glm::vec3(0.0f, 0.0f, -1.0f);
};
