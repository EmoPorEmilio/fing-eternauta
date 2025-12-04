#pragma once

#include "Entity.h"
#include "ComponentPool.h"
#include <vector>
#include <memory>
#include <unordered_map>
#include <functional>
#include <algorithm>
#include <tuple>
#include <type_traits>

namespace ecs {

// Forward declaration for view
template<typename... Components>
class View;

class Registry {
public:
    Registry() = default;
    ~Registry() = default;

    // Non-copyable, movable
    Registry(const Registry&) = delete;
    Registry& operator=(const Registry&) = delete;
    Registry(Registry&&) = default;
    Registry& operator=(Registry&&) = default;

    // Create a new entity
    Entity create() {
        EntityID id;

        if (!m_freeList.empty()) {
            // Reuse entity slot with incremented generation
            uint32_t index = m_freeList.back();
            m_freeList.pop_back();
            uint16_t generation = m_generations[index];
            id = makeEntityID(index, generation);
        } else {
            // Allocate new entity slot
            uint32_t index = static_cast<uint32_t>(m_generations.size());
            m_generations.push_back(0);
            id = makeEntityID(index, 0);
        }

        m_entityCount++;
        return Entity(id);
    }

    // Destroy an entity and all its components
    void destroy(Entity entity) {
        if (!isValid(entity)) return;

        uint32_t index = entity.index();

        // Remove all components from this entity
        for (auto& [typeId, pool] : m_pools) {
            pool->remove(entity.id);
        }

        // Increment generation to invalidate old references
        m_generations[index]++;

        // Add to free list for reuse
        m_freeList.push_back(index);
        m_entityCount--;
    }

    // Check if entity is still valid
    bool isValid(Entity entity) const {
        if (entity.id == INVALID_ENTITY) return false;
        uint32_t index = entity.index();
        if (index >= m_generations.size()) return false;
        return entity.generation() == m_generations[index];
    }

    // Add component to entity
    template<typename T, typename... Args>
    T& add(Entity entity, Args&&... args) {
        auto& pool = getOrCreatePool<T>();
        return pool.add(entity.id, std::forward<Args>(args)...);
    }

    // Remove component from entity
    template<typename T>
    void remove(Entity entity) {
        auto pool = tryGetPool<T>();
        if (pool) {
            pool->remove(entity.id);
        }
    }

    // Check if entity has component
    template<typename T>
    bool has(Entity entity) const {
        auto pool = tryGetPool<T>();
        return pool && pool->has(entity.id);
    }

    // Get component from entity (assumes entity has component)
    template<typename T>
    T& get(Entity entity) {
        return getPool<T>().get(entity.id);
    }

    template<typename T>
    const T& get(Entity entity) const {
        return getPool<T>().get(entity.id);
    }

    // Try to get component, returns nullptr if entity doesn't have it
    template<typename T>
    T* tryGet(Entity entity) {
        auto pool = tryGetPool<T>();
        return pool ? pool->tryGet(entity.id) : nullptr;
    }

    template<typename T>
    const T* tryGet(Entity entity) const {
        auto pool = tryGetPool<T>();
        return pool ? pool->tryGet(entity.id) : nullptr;
    }

    // Get component pool for type
    template<typename T>
    ComponentPool<T>& getPool() {
        return getOrCreatePool<T>();
    }

    template<typename T>
    const ComponentPool<T>& getPool() const {
        size_t typeId = getComponentTypeID<T>();
        auto it = m_pools.find(typeId);
        assert(it != m_pools.end() && "Component pool not found");
        return *static_cast<ComponentPool<T>*>(it->second.get());
    }

    // Create a view for iterating entities with specific components
    template<typename... Components>
    View<Components...> view();

    // Get total entity count
    size_t entityCount() const { return m_entityCount; }

    // Clear all entities and components
    void clear() {
        for (auto& [typeId, pool] : m_pools) {
            pool->clear();
        }
        m_generations.clear();
        m_freeList.clear();
        m_entityCount = 0;
    }

    // Iterate all entities with specific components (callback-based)
    template<typename... Components, typename Func>
    void each(Func&& func) {
        view<Components...>().each(std::forward<Func>(func));
    }

private:
    template<typename T>
    ComponentPool<T>& getOrCreatePool() {
        size_t typeId = getComponentTypeID<T>();
        auto it = m_pools.find(typeId);
        if (it == m_pools.end()) {
            auto [insertIt, _] = m_pools.emplace(typeId, std::make_unique<ComponentPool<T>>());
            return *static_cast<ComponentPool<T>*>(insertIt->second.get());
        }
        return *static_cast<ComponentPool<T>*>(it->second.get());
    }

    template<typename T>
    ComponentPool<T>* tryGetPool() {
        size_t typeId = getComponentTypeID<T>();
        auto it = m_pools.find(typeId);
        if (it == m_pools.end()) return nullptr;
        return static_cast<ComponentPool<T>*>(it->second.get());
    }

    template<typename T>
    const ComponentPool<T>* tryGetPool() const {
        size_t typeId = getComponentTypeID<T>();
        auto it = m_pools.find(typeId);
        if (it == m_pools.end()) return nullptr;
        return static_cast<const ComponentPool<T>*>(it->second.get());
    }

    std::unordered_map<size_t, std::unique_ptr<IComponentPool>> m_pools;
    std::vector<uint16_t> m_generations;  // Generation counter per entity slot
    std::vector<uint32_t> m_freeList;     // Reusable entity indices
    size_t m_entityCount = 0;
};

// View for iterating entities with specific components
template<typename... Components>
class View {
public:
    explicit View(Registry& registry) : m_registry(registry) {}

    // Iterate all entities with the specified components
    template<typename Func>
    void each(Func&& func) {
        // Use the first component's pool for iteration
        // (could optimize to find smallest, but this is simpler)
        auto& firstPool = m_registry.getPool<std::tuple_element_t<0, std::tuple<Components...>>>();

        for (size_t i = 0; i < firstPool.size(); ++i) {
            EntityID entityId = firstPool.getEntityAt(i);
            Entity entity(entityId);

            // Check if entity has all required components
            if ((m_registry.has<Components>(entity) && ...)) {
                if constexpr (std::is_invocable_v<Func, Entity, Components&...>) {
                    func(entity, m_registry.get<Components>(entity)...);
                } else {
                    func(m_registry.get<Components>(entity)...);
                }
            }
        }
    }

    // Get first entity matching the view (or invalid entity if none)
    Entity front() {
        auto& firstPool = m_registry.getPool<std::tuple_element_t<0, std::tuple<Components...>>>();

        for (size_t i = 0; i < firstPool.size(); ++i) {
            EntityID entityId = firstPool.getEntityAt(i);
            Entity entity(entityId);
            if ((m_registry.has<Components>(entity) && ...)) {
                return entity;
            }
        }
        return Entity(INVALID_ENTITY);
    }

private:
    Registry& m_registry;
};

template<typename... Components>
View<Components...> Registry::view() {
    return View<Components...>(*this);
}

} // namespace ecs
