#pragma once

#include "../ecs/System.h"
#include "../components/TransformComponent.h"
#include "../components/RenderableComponent.h"
#include "../components/LODComponent.h"
#include <glm/glm.hpp>

namespace ecs {

// Updates visibility based on distance culling and frustum culling
class CullingSystem : public System {
public:
    // Set culling distance
    void setCullDistance(float distance) { m_cullDistance = distance; }
    float getCullDistance() const { return m_cullDistance; }

    // Enable/disable culling
    void setCullingEnabled(bool enabled) { m_cullingEnabled = enabled; }
    bool isCullingEnabled() const { return m_cullingEnabled; }

    void update(Registry& registry, float deltaTime) override {
        (void)deltaTime;

        if (!m_cullingEnabled) {
            // Culling disabled - make everything visible
            registry.each<RenderableComponent>([](RenderableComponent& renderable) {
                renderable.visible = true;
            });
            return;
        }

        // Distance-based culling using LOD distance
        registry.each<LODComponent, RenderableComponent>([&](LODComponent& lod, RenderableComponent& renderable) {
            renderable.visible = (lod.distanceToCamera <= m_cullDistance);
        });

        // For entities without LOD but with Transform, calculate distance
        registry.each<TransformComponent, RenderableComponent>([&](Entity entity, TransformComponent& transform, RenderableComponent& renderable) {
            // Skip if entity has LOD component (already handled above)
            if (registry.has<LODComponent>(entity)) return;

            float distance = glm::length(transform.position - m_cameraPosition);
            renderable.visible = (distance <= m_cullDistance);
        });
    }

    // Set camera position for culling calculations
    void setCameraPosition(const glm::vec3& pos) {
        m_cameraPosition = pos;
    }

    const char* name() const override { return "CullingSystem"; }

private:
    glm::vec3 m_cameraPosition{0.0f};
    float m_cullDistance = 500.0f;
    bool m_cullingEnabled = true;
};

} // namespace ecs
