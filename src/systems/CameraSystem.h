#pragma once

#include "../ecs/System.h"
#include "../components/TransformComponent.h"
#include "../components/CameraComponent.h"
#include <glm/glm.hpp>

namespace ecs {

// Updates camera direction vectors and matrices
class CameraSystem : public System {
public:
    void update(Registry& registry, float deltaTime) override {
        (void)deltaTime;

        registry.each<TransformComponent, CameraComponent>(
            [](TransformComponent& transform, CameraComponent& camera) {
                if (!camera.isActive) return;

                // Update direction vectors if dirty
                if (camera.matricesDirty) {
                    camera.updateVectors();
                }

                // Update view matrix
                camera.viewMatrix = camera.calculateViewMatrix(transform.position);

                // Update projection matrix
                camera.projectionMatrix = camera.calculateProjectionMatrix();

                camera.matricesDirty = false;
            });
    }

    const char* name() const override { return "CameraSystem"; }

    // Get active camera entity
    Entity getActiveCamera(Registry& registry) {
        Entity activeCamera(INVALID_ENTITY);

        registry.each<CameraComponent>([&](Entity entity, CameraComponent& camera) {
            if (camera.isActive) {
                activeCamera = entity;
            }
        });

        return activeCamera;
    }

    // Get view matrix from active camera
    glm::mat4 getViewMatrix(Registry& registry) {
        glm::mat4 view(1.0f);

        registry.each<CameraComponent>([&](CameraComponent& camera) {
            if (camera.isActive) {
                view = camera.viewMatrix;
            }
        });

        return view;
    }

    // Get projection matrix from active camera
    glm::mat4 getProjectionMatrix(Registry& registry) {
        glm::mat4 proj(1.0f);

        registry.each<CameraComponent>([&](CameraComponent& camera) {
            if (camera.isActive) {
                proj = camera.projectionMatrix;
            }
        });

        return proj;
    }

    // Get camera position from active camera
    glm::vec3 getCameraPosition(Registry& registry) {
        glm::vec3 pos(0.0f);

        registry.each<TransformComponent, CameraComponent>(
            [&](TransformComponent& transform, CameraComponent& camera) {
                if (camera.isActive) {
                    pos = transform.position;
                }
            });

        return pos;
    }

    // Get camera front direction from active camera
    glm::vec3 getCameraFront(Registry& registry) {
        glm::vec3 front(0.0f, 0.0f, -1.0f);

        registry.each<CameraComponent>([&](CameraComponent& camera) {
            if (camera.isActive) {
                front = camera.front;
            }
        });

        return front;
    }
};

} // namespace ecs
