#pragma once

#include "../ecs/System.h"
#include "../components/TransformComponent.h"
#include "../components/LODComponent.h"
#include "../components/BatchGroupComponent.h"
#include <glm/glm.hpp>

namespace ecs {

// Updates LOD levels based on distance to camera
class LODSystem : public System {
public:
    // Set camera position for distance calculations
    void setCameraPosition(const glm::vec3& pos) {
        m_cameraPosition = pos;
    }

    // Enable/disable LOD (when disabled, all entities use HIGH LOD)
    void setLODEnabled(bool enabled) { m_lodEnabled = enabled; }
    bool isLODEnabled() const { return m_lodEnabled; }

    // Update frequency - only update every N frames for performance
    void setUpdateFrequency(int frames) { m_updateFrequency = frames; }

    void update(Registry& registry, float deltaTime) override {
        (void)deltaTime;

        // Only update LOD every N frames for performance
        m_frameCounter++;
        bool shouldUpdate = (m_frameCounter >= m_updateFrequency);
        if (shouldUpdate) {
            m_frameCounter = 0;
        }

        // Update all entities with Transform + LOD components
        registry.each<TransformComponent, LODComponent>([&](Entity entity, TransformComponent& transform, LODComponent& lod) {
            // Always update distance
            lod.distanceToCamera = glm::length(transform.position - m_cameraPosition);

            if (shouldUpdate) {
                LODLevel previousLevel = lod.currentLevel;

                if (m_lodEnabled) {
                    lod.updateLOD();
                } else {
                    lod.currentLevel = LODLevel::HIGH;
                }

                // If LOD changed, update BatchGroup if present
                if (lod.currentLevel != previousLevel) {
                    if (auto* batch = registry.tryGet<BatchGroupComponent>(entity)) {
                        batch->lodLevel = lod.currentLevel;
                        batch->batchDirty = true;
                    }
                }
            }
        });
    }

    const char* name() const override { return "LODSystem"; }

private:
    glm::vec3 m_cameraPosition{0.0f};
    bool m_lodEnabled = true;
    int m_updateFrequency = 10; // Update every 10 frames by default
    int m_frameCounter = 0;
};

} // namespace ecs
