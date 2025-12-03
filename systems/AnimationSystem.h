#pragma once

#include "../ecs/System.h"
#include "../components/AnimationComponent.h"

namespace ecs {

// Advances animation time for all entities with AnimationComponent
// Note: The actual joint matrix computation is delegated to the model's
// evaluateAnimation() method since it has access to the animation data.
// This system handles timing and state management.
class AnimationSystem : public System {
public:
    void update(Registry& registry, float deltaTime) override {
        // Advance time for all playing animations
        registry.each<AnimationComponent>([deltaTime](AnimationComponent& anim) {
            anim.advanceTime(deltaTime);
        });
    }

    const char* name() const override { return "AnimationSystem"; }

    // Global animation speed multiplier (affects all animations)
    void setGlobalSpeed(float speed) { m_globalSpeed = speed; }
    float getGlobalSpeed() const { return m_globalSpeed; }

    // Pause/resume all animations
    void pauseAll(Registry& registry) {
        registry.each<AnimationComponent>([](AnimationComponent& anim) {
            if (anim.state == AnimationState::PLAYING) {
                anim.pause();
            }
        });
    }

    void resumeAll(Registry& registry) {
        registry.each<AnimationComponent>([](AnimationComponent& anim) {
            if (anim.state == AnimationState::PAUSED) {
                anim.play();
            }
        });
    }

private:
    float m_globalSpeed = 1.0f;
};

} // namespace ecs
