#pragma once
#include "../Entity.h"

struct FollowTarget {
    Entity target = NULL_ENTITY;
    float distance = 2.2f;      // Distance behind character
    float height = 1.2f;        // Height above character
    float shoulderOffset = 2.4f; // Offset to the right (shoulder view)
    float lookAhead = 5.0f;     // How far ahead of character to look
    float yaw = 0.0f;           // Facing direction (controls character + camera)
    float pitch = 10.0f;        // Vertical look angle
    float sensitivity = 0.3f;
};
