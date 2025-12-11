#pragma once
#include "../../NurbsCurve.h"
#include "../Registry.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// Cinematic camera sequences using NURBS curves
class CinematicSystem {
public:
    CinematicSystem() = default;

    void setCameraPath(const NurbsCurve& path) {
        m_cameraPath = path;
    }

    void setLookAtTarget(Entity target) {
        m_lookAtTarget = target;
    }

    void setCharacterEntity(Entity character) {
        m_characterEntity = character;
    }

    void setCharacterYaw(float startYaw, float endYaw) {
        m_characterStartYaw = startYaw;
        m_characterEndYaw = endYaw;
    }

    void setFinalLookAt(const glm::vec3& pos) {
        m_finalLookAt = pos;
        m_hasFinalLookAt = true;
    }

    void setDuration(float seconds) {
        m_duration = seconds;
    }

    void start(Registry& registry) {
        m_progress = 0.0f;
        m_isPlaying = true;
        m_isComplete = false;

        if (m_characterEntity != NULL_ENTITY) {
            setCharacterRotation(registry, m_characterStartYaw);
        }
    }

    void stop(Registry& registry) {
        m_isPlaying = false;
        m_isComplete = true;
        m_progress = 1.0f;

        if (m_characterEntity != NULL_ENTITY) {
            setCharacterRotation(registry, m_characterEndYaw);
        }
    }

    bool update(Registry& registry, float dt) {
        if (!m_isPlaying) return false;

        m_progress += dt / m_duration;

        if (m_progress >= 1.0f) {
            m_progress = 1.0f;
            m_isPlaying = false;
            m_isComplete = true;

            if (m_characterEntity != NULL_ENTITY) {
                setCharacterRotation(registry, m_characterEndYaw);
            }
            return false;
        }

        float easedT = easeInOutCubic(m_progress);

        Entity camEntity = registry.getActiveCamera();
        if (camEntity != NULL_ENTITY) {
            auto* camTransform = registry.getTransform(camEntity);
            if (camTransform) {
                camTransform->position = m_cameraPath.evaluate(easedT);
            }
        }

        if (m_characterEntity != NULL_ENTITY) {
            float yaw = glm::mix(m_characterStartYaw, m_characterEndYaw, easedT);
            setCharacterRotation(registry, yaw);
        }

        return true;
    }

    glm::mat4 getViewMatrix(Registry& registry) const {
        Entity camEntity = registry.getActiveCamera();
        if (camEntity == NULL_ENTITY) return glm::mat4(1.0f);

        auto* camTransform = registry.getTransform(camEntity);
        if (!camTransform) return glm::mat4(1.0f);

        glm::vec3 cameraPos = camTransform->position;
        glm::vec3 lookAt;

        glm::vec3 characterLookAt(0.0f, 1.0f, 0.0f);
        if (m_lookAtTarget != NULL_ENTITY) {
            auto* targetTransform = registry.getTransform(m_lookAtTarget);
            if (targetTransform) {
                characterLookAt = targetTransform->position + glm::vec3(0.0f, 1.0f, 0.0f);
            }
        }

        // Blend from character to final gameplay look-at in last 30%
        if (m_hasFinalLookAt) {
            float easedT = easeInOutCubic(m_progress);
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

    glm::vec3 getCurrentCameraPosition() const {
        float easedT = easeInOutCubic(m_progress);
        return m_cameraPath.evaluate(easedT);
    }

    bool isPlaying() const { return m_isPlaying; }
    bool isComplete() const { return m_isComplete; }
    float progress() const { return m_progress; }
    glm::vec3 getFinalPosition() const { return m_cameraPath.evaluate(1.0f); }
    glm::vec3 getFinalLookAt() const { return m_finalLookAt; }

private:
    NurbsCurve m_cameraPath;
    Entity m_lookAtTarget = NULL_ENTITY;
    Entity m_characterEntity = NULL_ENTITY;
    glm::vec3 m_finalLookAt = glm::vec3(0.0f, 1.0f, 0.0f);
    bool m_hasFinalLookAt = false;

    float m_characterStartYaw = 0.0f;
    float m_characterEndYaw = 0.0f;
    float m_duration = 3.0f;
    float m_progress = 0.0f;
    bool m_isPlaying = false;
    bool m_isComplete = false;

    // Septic (power 7) ease for dramatic acceleration/deceleration
    static float easeInOutCubic(float t) {
        if (t < 0.5f) {
            return 64.0f * t * t * t * t * t * t * t;
        } else {
            float f = 2.0f * t - 2.0f;
            return 0.5f * f * f * f * f * f * f * f + 1.0f;
        }
    }

    void setCharacterRotation(Registry& registry, float yaw) {
        auto* facing = registry.getFacingDirection(m_characterEntity);
        if (facing) {
            facing->yaw = yaw;
        }

        auto* transform = registry.getTransform(m_characterEntity);
        if (transform) {
            // Model faces +Z by default, add PI to align with yaw
            float yawRad = glm::radians(yaw);
            transform->rotation = glm::angleAxis(yawRad + glm::pi<float>(), glm::vec3(0.0f, 1.0f, 0.0f));
        }
    }
};
