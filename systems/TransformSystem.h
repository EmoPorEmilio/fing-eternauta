#pragma once

#include "../ecs/System.h"
#include "../components/TransformComponent.h"

namespace ecs {

// Updates model matrices for entities with dirty transforms
class TransformSystem : public System {
public:
    void update(Registry& registry, float deltaTime) override {
        (void)deltaTime; // Not needed for transform updates

        // Update all dirty transforms
        registry.each<TransformComponent>([](TransformComponent& transform) {
            transform.updateModelMatrix();
        });
    }

    const char* name() const override { return "TransformSystem"; }
};

} // namespace ecs
