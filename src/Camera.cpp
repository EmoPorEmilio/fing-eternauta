#include "Camera.h"
#include <SDL.h>
#include <glm/gtc/matrix_transform.hpp>

Camera::Camera(float x, float y, float z)
    : position(x, y, z), front(glm::vec3(0.0f, 0.0f, -1.0f)), worldUp(glm::vec3(0.0f, 1.0f, 0.0f)),
      yaw(-90.0f), pitch(0.0f), movementSpeed(30.0f), mouseSensitivity(0.15f)
{
    updateCameraVectors();
}

void Camera::update(float deltaTime)
{
    // Camera update logic (currently just position tracking)
}

void Camera::handleInput(const Uint8 *keys, float deltaTime)
{
    float velocity = movementSpeed * deltaTime;

    if (keys[SDL_SCANCODE_W])
        position += front * velocity;
    if (keys[SDL_SCANCODE_S])
        position -= front * velocity;
    if (keys[SDL_SCANCODE_A])
        position -= right * velocity;
    if (keys[SDL_SCANCODE_D])
        position += right * velocity;
    if (keys[SDL_SCANCODE_Q])
        position -= up * velocity; // up
    if (keys[SDL_SCANCODE_E])
        position += up * velocity; // down
}

void Camera::handleMouseInput(float xoffset, float yoffset)
{
    xoffset *= mouseSensitivity;
    yoffset *= mouseSensitivity;

    yaw += xoffset;
    pitch += yoffset;

    // Keep yaw in reasonable range but allow unlimited rotation
    // This prevents floating point precision issues with very large values
    while (yaw > 360.0f)
        yaw -= 360.0f;
    while (yaw < -360.0f)
        yaw += 360.0f;

    // Constrain pitch to prevent screen flipping
    if (pitch > 89.0f)
        pitch = 89.0f;
    if (pitch < -89.0f)
        pitch = -89.0f;

    updateCameraVectors();
}

void Camera::updateCameraVectors()
{
    // Calculate new front vector
    glm::vec3 newFront;
    newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
    newFront.y = sin(glm::radians(pitch));
    newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
    front = glm::normalize(newFront);

    // Recalculate right and up vectors
    right = glm::normalize(glm::cross(front, worldUp));
    up = glm::normalize(glm::cross(right, front));
}

glm::mat4 Camera::getViewMatrix() const
{
    return glm::lookAt(position, position + front, up);
}
