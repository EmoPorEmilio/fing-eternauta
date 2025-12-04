#pragma once

#include <glm/glm.hpp>

namespace ecs {

// Particle type for different particle systems
enum class ParticleType : uint8_t {
    SNOW,
    RAIN,
    DUST,
    SPARK,
    CUSTOM
};

struct ParticleComponent {
    ParticleType type = ParticleType::SNOW;

    // Velocity and physics
    glm::vec3 velocity{0.0f, -1.0f, 0.0f};
    glm::vec3 acceleration{0.0f};
    float drag = 0.0f;

    // Previous position (for motion blur, interpolation)
    glm::vec3 prevPosition{0.0f};

    // Lifetime
    float lifetime = 10.0f;       // Total lifetime in seconds
    float age = 0.0f;             // Current age
    bool alive = true;

    // Per-particle variation
    float seed = 0.0f;            // Random seed for variation
    float size = 1.0f;            // Particle size multiplier

    // Snow-specific properties
    float fallSpeed = 1.0f;       // Base fall speed
    bool settled = false;         // On ground?
    float settleTimer = 0.0f;     // Time remaining settled

    // Visual
    glm::vec4 color{1.0f};
    float alpha = 1.0f;

    ParticleComponent() = default;

    // Snow particle constructor
    static ParticleComponent Snow(const glm::vec3& pos, float speed, float randomSeed) {
        ParticleComponent p;
        p.type = ParticleType::SNOW;
        p.prevPosition = pos;
        p.fallSpeed = speed;
        p.seed = randomSeed;
        p.velocity = glm::vec3(0.0f, -speed, 0.0f);
        return p;
    }

    // Check if particle should be removed
    bool isDead() const {
        return !alive || age >= lifetime;
    }

    // Get normalized age (0-1)
    float getNormalizedAge() const {
        return lifetime > 0.0f ? age / lifetime : 1.0f;
    }

    // Update age and check death
    void updateAge(float deltaTime) {
        age += deltaTime;
        if (age >= lifetime) {
            alive = false;
        }
    }
};

} // namespace ecs
