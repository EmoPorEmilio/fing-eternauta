#pragma once

#include <glm/glm.hpp>
#include <SDL.h>

class Camera
{
public:
    Camera(float x = 0.0f, float y = 1.6f, float z = 3.0f);

    void update(float deltaTime);
    void handleInput(const Uint8 *keys, float deltaTime);
    void handleMouseInput(float xoffset, float yoffset);

    glm::mat4 getViewMatrix() const;
    glm::vec3 getPosition() const { return position; }
    glm::vec3 getFront() const { return front; }

    void setPosition(const glm::vec3 &pos) { position = pos; }
    void setMovementSpeed(float speed) { movementSpeed = speed; }

private:
    glm::vec3 position;
    glm::vec3 front;
    glm::vec3 up;
    glm::vec3 right;
    glm::vec3 worldUp;

    float yaw;
    float pitch;
    float movementSpeed;
    float mouseSensitivity;

    void updateCameraVectors();
};
