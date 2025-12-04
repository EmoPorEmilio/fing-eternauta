/**
 * @file ECSWorld.h
 * @brief Global Entity-Component-System singleton
 *
 * ECSWorld provides centralized access to the entity registry and system
 * scheduler. All managers create their entities through this singleton,
 * ensuring a unified view of the game world.
 *
 * Core Access:
 *   - ECSWorld::getRegistry() - Access entity/component storage
 *   - ECSWorld::getSystems() - Access system scheduler
 *
 * Lifecycle:
 *   1. ECSWorld::initialize() - Called once at startup (Application::initialize)
 *   2. ECSWorld::update(deltaTime) - Called each frame before render
 *   3. ECSWorld::shutdown() - Called at application exit
 *
 * Entity Management:
 *   auto& reg = ECSWorld::getRegistry();
 *   auto entity = reg.create();
 *   reg.add<TransformComponent>(entity, pos, rot, scale);
 *   reg.get<TransformComponent>(entity).position = newPos;
 *   reg.destroy(entity);
 *
 * System Registration:
 *   auto& systems = ECSWorld::getSystems();
 *   systems.add<LODSystem>();
 *   systems.add<CullingSystem>();
 *   // Systems update in registration order
 *
 * Thread Safety:
 *   Not thread-safe. All access should be from the main thread.
 *
 * @see ecs/Registry.h, ecs/System.h, ecs/Entity.h
 */
#pragma once

#include "ecs/Registry.h"
#include "ecs/System.h"

// Global ECS world - single source of truth for all entities and systems.
// Replaces per-manager Registry instances for unified entity management.
class ECSWorld {
public:
    // Access global registry (singleton)
    static ecs::Registry& getRegistry();

    // Access global system scheduler
    static ecs::SystemScheduler& getSystems();

    // Initialize the ECS world (call once at startup)
    static void initialize();

    // Update all systems (call once per frame)
    static void update(float deltaTime);

    // Shutdown and cleanup (call at application exit)
    static void shutdown();

    // Check if initialized
    static bool isInitialized();

private:
    ECSWorld() = default;
    ~ECSWorld() = default;

    // Non-copyable
    ECSWorld(const ECSWorld&) = delete;
    ECSWorld& operator=(const ECSWorld&) = delete;

    static ecs::Registry s_registry;
    static ecs::SystemScheduler s_systems;
    static bool s_initialized;
};
