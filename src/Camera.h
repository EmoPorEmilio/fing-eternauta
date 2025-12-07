#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <SDL.h>

struct Camera
{
    glm::vec3 position = glm::vec3(3.0f, 2.0f, 5.0f);
    glm::vec3 front = glm::vec3(0.0f, 0.0f, -1.0f);
    glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);

    float yaw = -90.0f;
    float pitch = -15.0f;
    float speed = 5.0f;
    float sensitivity = 0.1f;

    void updateVectors()
    {
        glm::vec3 f;
        f.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
        f.y = sin(glm::radians(pitch));
        f.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
        front = glm::normalize(f);
    }

    glm::mat4 getViewMatrix() const
    {
        return glm::lookAt(position, position + front, up);
    }

    void processMouseMovement(float xoffset, float yoffset)
    {
        yaw += xoffset * sensitivity;
        pitch -= yoffset * sensitivity;

        if (pitch > 89.0f) pitch = 89.0f;
        if (pitch < -89.0f) pitch = -89.0f;

        updateVectors();
    }

    void processKeyboard(const Uint8* keys, float deltaTime)
    {
        float velocity = speed * deltaTime;
        glm::vec3 right = glm::normalize(glm::cross(front, up));

        if (keys[SDL_SCANCODE_W]) position += front * velocity;
        if (keys[SDL_SCANCODE_S]) position -= front * velocity;
        if (keys[SDL_SCANCODE_A]) position -= right * velocity;
        if (keys[SDL_SCANCODE_D]) position += right * velocity;
        if (keys[SDL_SCANCODE_SPACE]) position += up * velocity;
        if (keys[SDL_SCANCODE_LSHIFT]) position -= up * velocity;
    }
};
