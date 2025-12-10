#pragma once
#include "../../NurbsCurve.h"
#include "../Registry.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Manages cinematic camera sequences using NURBS curves
class CinematicSystem {
public:
    CinematicSystem() = default;

    // Set up the camera path curve
    void setCameraPath(const NurbsCurve& path) {
        m_cameraPath = path;
    }

    // Set the target the camera should look at during the cinematic
    void setLookAtTarget(Entity target) {
        m_lookAtTarget = target;
    }

    // Set the character entity to animate facing direction
    void setCharacterEntity(Entity character) {
        m_characterEntity = character;
    }

    // Set start and end yaw for character facing animation
    void setCharacterYaw(float startYaw, float endYaw) {
        m_characterStartYaw = startYaw;
        m_characterEndYaw = endYaw;
    }

    // Set the final look-at position (for smooth blend to gameplay camera)
    void setFinalLookAt(const glm::vec3& pos) {
        m_finalLookAt = pos;
        m_hasFinalLookAt = true;
    }

    // Set cinematic duration in seconds
    void setDuration(float seconds) {
        m_duration = seconds;
    }

    // Start the cinematic
    void start(Registry& registry) {
        m_progress = 0.0f;
        m_isPlaying = true;
        m_isComplete = false;

        // Set character to start facing (both FacingDirection and Transform)
        if (m_characterEntity != NULL_ENTITY) {
            setCharacterRotation(registry, m_characterStartYaw);
        }
    }

    // Stop/skip the cinematic
    void stop(Registry& registry) {
        m_isPlaying = false;
        m_isComplete = true;
        m_progress = 1.0f;

        // Ensure character ends at final yaw when skipped
        if (m_characterEntity != NULL_ENTITY) {
            setCharacterRotation(registry, m_characterEndYaw);
        }
    }

    // Update the cinematic (call each frame)
    // Returns true while playing, false when complete
    bool update(Registry& registry, float dt) {
        if (!m_isPlaying) return false;

        m_progress += dt / m_duration;

        if (m_progress >= 1.0f) {
            m_progress = 1.0f;
            m_isPlaying = false;
            m_isComplete = true;

            // Ensure character ends at exact final yaw
            if (m_characterEntity != NULL_ENTITY) {
                setCharacterRotation(registry, m_characterEndYaw);
            }
            return false;
        }

        // Apply easing for smoother start/end
        float easedT = easeInOutCubic(m_progress);

        // Update camera position along the curve
        Entity camEntity = registry.getActiveCamera();
        if (camEntity != NULL_ENTITY) {
            auto* camTransform = registry.getTransform(camEntity);
            if (camTransform) {
                camTransform->position = m_cameraPath.evaluate(easedT);
            }
        }

        // Update character facing direction and transform rotation
        if (m_characterEntity != NULL_ENTITY) {
            float yaw = glm::mix(m_characterStartYaw, m_characterEndYaw, easedT);
            setCharacterRotation(registry, yaw);
        }

        return true;
    }

    // Get the view matrix for rendering during cinematic
    glm::mat4 getViewMatrix(Registry& registry) const {
        Entity camEntity = registry.getActiveCamera();
        if (camEntity == NULL_ENTITY) return glm::mat4(1.0f);

        auto* camTransform = registry.getTransform(camEntity);
        if (!camTransform) return glm::mat4(1.0f);

        glm::vec3 cameraPos = camTransform->position;
        glm::vec3 lookAt;

        // Get the character's look-at point (at their position + eye level)
        glm::vec3 characterLookAt(0.0f, 1.0f, 0.0f);
        if (m_lookAtTarget != NULL_ENTITY) {
            auto* targetTransform = registry.getTransform(m_lookAtTarget);
            if (targetTransform) {
                characterLookAt = targetTransform->position + glm::vec3(0.0f, 1.0f, 0.0f);
            }
        }

        // Blend from character look-at to final gameplay look-at over the cinematic
        if (m_hasFinalLookAt) {
            float easedT = easeInOutCubic(m_progress);
            // Start blending to final look-at in the last 30% of the cinematic
            float blendStart = 0.7f;
            if (easedT > blendStart) {
                float blendT = (easedT - blendStart) / (1.0f - blendStart);
                lookAt = glm::mix(characterLookAt, m_finalLookAt, blendT);
            } else {
                lookAt = characterLookAt;
            }
        } else {
            lookAt = characterLookAt;
        }

        return glm::lookAt(cameraPos, lookAt, glm::vec3(0.0f, 1.0f, 0.0f));
    }

    // Get current camera position (for other systems that need it)
    glm::vec3 getCurrentCameraPosition() const {
        float easedT = easeInOutCubic(m_progress);
        return m_cameraPath.evaluate(easedT);
    }

    bool isPlaying() const { return m_isPlaying; }
    bool isComplete() const { return m_isComplete; }
    float progress() const { return m_progress; }

    // Get final camera position (for smooth handoff to gameplay)
    glm::vec3 getFinalPosition() const {
        return m_cameraPath.evaluate(1.0f);
    }

    // Get final look-at position
    glm::vec3 getFinalLookAt() const {
        return m_finalLookAt;
    }

private:
    NurbsCurve m_cameraPath;
    Entity m_lookAtTarget = NULL_ENTITY;
    Entity m_characterEntity = NULL_ENTITY;
    glm::vec3 m_finalLookAt = glm::vec3(0.0f, 1.0f, 0.0f);
    bool m_hasFinalLookAt = false;

    float m_characterStartYaw = 0.0f;  // Character facing at cinematic start
    float m_characterEndYaw = 0.0f;    // Character facing at cinematic end

    float m_duration = 3.0f;  // seconds
    float m_progress = 0.0f;  // 0 to 1
    bool m_isPlaying = false;
    bool m_isComplete = false;

    // Septic ease in-out for extreme acceleration/deceleration and very fast peak speed
    static float easeInOutCubic(float t) {
        // Using septic (power of 7) for very dramatic acceleration/deceleration
        if (t < 0.5f) {
            return 64.0f * t * t * t * t * t * t * t;
        } else {
            float f = 2.0f * t - 2.0f;
            return 0.5f * f * f * f * f * f * f * f + 1.0f;
        }
    }

    // Set character's yaw in both FacingDirection and Transform rotation
    void setCharacterRotation(Registry& registry, float yaw) {
        auto* facing = registry.getFacingDirection(m_characterEntity);
        if (facing) {
            facing->yaw = yaw;
        }

        auto* transform = registry.getTransform(m_characterEntity);
        if (transform) {
            // Same formula as PlayerMovementSystem: model faces +Z by default, so add PI
            float yawRad = glm::radians(yaw);
            transform->rotation = glm::angleAxis(yawRad + glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
        }
    }
};
