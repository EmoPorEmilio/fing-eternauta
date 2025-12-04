#include "ECSWorld.h"
#include <iostream>

// Include systems for registration
#include "systems/TransformSystem.h"
#include "systems/LODSystem.h"
#include "systems/CullingSystem.h"
#include "systems/ParticleSystem.h"
#include "systems/LightSystem.h"
#include "systems/AnimationSystem.h"

// Static member definitions
ecs::Registry ECSWorld::s_registry;
ecs::SystemScheduler ECSWorld::s_systems;
bool ECSWorld::s_initialized = false;

ecs::Registry& ECSWorld::getRegistry() {
    return s_registry;
}

ecs::SystemScheduler& ECSWorld::getSystems() {
    return s_systems;
}

void ECSWorld::initialize() {
    if (s_initialized) {
        std::cerr << "[ECSWorld] Warning: Already initialized" << std::endl;
        return;
    }

    s_registry.clear();

    // Register core systems in update order
    // Note: Order matters! Earlier systems run first.
    s_systems.addSystem<ecs::TransformSystem>();   // Update model matrices
    s_systems.addSystem<ecs::LODSystem>();         // Update LOD levels based on distance
    s_systems.addSystem<ecs::CullingSystem>();     // Frustum/distance culling
    s_systems.addSystem<ecs::AnimationSystem>();   // Animation playback
    s_systems.addSystem<ecs::ParticleSystem>();    // Particle simulation
    s_systems.addSystem<ecs::LightSystem>();       // Light updates

    // Initialize all systems
    s_systems.init(s_registry);

    s_initialized = true;

    std::cout << "[ECSWorld] Initialized with " << s_systems.getSystems().size() << " systems" << std::endl;
}

void ECSWorld::update(float deltaTime) {
    if (!s_initialized) {
        return;
    }
    s_systems.update(s_registry, deltaTime);
}

void ECSWorld::shutdown() {
    if (!s_initialized) {
        return;
    }

    // Clear all entities and components
    s_registry.clear();
    s_initialized = false;

    std::cout << "[ECSWorld] Shutdown" << std::endl;
}

bool ECSWorld::isInitialized() {
    return s_initialized;
}
