#pragma once

#include "Entity.h"
#include <vector>
#include <cassert>
#include <type_traits>

namespace ecs {

// Type ID generator for components
inline size_t getNextComponentTypeID() {
    static size_t nextID = 0;
    return nextID++;
}

template<typename T>
inline size_t getComponentTypeID() {
    static size_t typeID = getNextComponentTypeID();
    return typeID;
}

// Base class for type-erased component pool storage
class IComponentPool {
public:
    virtual ~IComponentPool() = default;
    virtual void remove(EntityID entity) = 0;
    virtual bool has(EntityID entity) const = 0;
    virtual size_t size() const = 0;
    virtual void clear() = 0;
};

// Sparse set component pool for cache-friendly iteration
// Provides O(1) add, remove, and lookup operations
template<typename T>
class ComponentPool : public IComponentPool {
public:
    static_assert(std::is_default_constructible_v<T>, "Component must be default constructible");

    ComponentPool() {
        // Reserve reasonable initial capacity
        m_dense.reserve(1024);
        m_denseToEntity.reserve(1024);
    }

    // Add component to entity, returns reference to component
    template<typename... Args>
    T& add(EntityID entity, Args&&... args) {
        uint32_t index = getEntityIndex(entity);

        // Ensure sparse array is large enough
        if (index >= m_sparse.size()) {
            m_sparse.resize(index + 1, INVALID_INDEX);
        }

        // Check if entity already has this component
        if (m_sparse[index] != INVALID_INDEX) {
            // Replace existing component
            m_dense[m_sparse[index]] = T(std::forward<Args>(args)...);
            return m_dense[m_sparse[index]];
        }

        // Add new component
        size_t denseIndex = m_dense.size();
        m_sparse[index] = static_cast<uint32_t>(denseIndex);
        m_dense.emplace_back(std::forward<Args>(args)...);
        m_denseToEntity.push_back(entity);

        return m_dense.back();
    }

    // Remove component from entity
    void remove(EntityID entity) override {
        uint32_t index = getEntityIndex(entity);

        if (index >= m_sparse.size() || m_sparse[index] == INVALID_INDEX) {
            return; // Entity doesn't have this component
        }

        uint32_t denseIndex = m_sparse[index];
        uint32_t lastDenseIndex = static_cast<uint32_t>(m_dense.size() - 1);

        if (denseIndex != lastDenseIndex) {
            // Swap with last element
            m_dense[denseIndex] = std::move(m_dense[lastDenseIndex]);
            m_denseToEntity[denseIndex] = m_denseToEntity[lastDenseIndex];

            // Update sparse array for swapped element
            uint32_t swappedEntityIndex = getEntityIndex(m_denseToEntity[denseIndex]);
            m_sparse[swappedEntityIndex] = denseIndex;
        }

        // Remove last element
        m_dense.pop_back();
        m_denseToEntity.pop_back();
        m_sparse[index] = INVALID_INDEX;
    }

    // Check if entity has this component
    bool has(EntityID entity) const override {
        uint32_t index = getEntityIndex(entity);
        return index < m_sparse.size() && m_sparse[index] != INVALID_INDEX;
    }

    // Get component for entity (assumes entity has component)
    T& get(EntityID entity) {
        uint32_t index = getEntityIndex(entity);
        assert(index < m_sparse.size() && m_sparse[index] != INVALID_INDEX);
        return m_dense[m_sparse[index]];
    }

    const T& get(EntityID entity) const {
        uint32_t index = getEntityIndex(entity);
        assert(index < m_sparse.size() && m_sparse[index] != INVALID_INDEX);
        return m_dense[m_sparse[index]];
    }

    // Try to get component, returns nullptr if entity doesn't have it
    T* tryGet(EntityID entity) {
        uint32_t index = getEntityIndex(entity);
        if (index >= m_sparse.size() || m_sparse[index] == INVALID_INDEX) {
            return nullptr;
        }
        return &m_dense[m_sparse[index]];
    }

    const T* tryGet(EntityID entity) const {
        uint32_t index = getEntityIndex(entity);
        if (index >= m_sparse.size() || m_sparse[index] == INVALID_INDEX) {
            return nullptr;
        }
        return &m_dense[m_sparse[index]];
    }

    // Number of components stored
    size_t size() const override { return m_dense.size(); }

    // Clear all components
    void clear() override {
        m_dense.clear();
        m_denseToEntity.clear();
        m_sparse.clear();
    }

    // Iteration support - iterate over dense array for cache efficiency
    auto begin() { return m_dense.begin(); }
    auto end() { return m_dense.end(); }
    auto begin() const { return m_dense.begin(); }
    auto end() const { return m_dense.end(); }

    // Get entity at dense index
    EntityID getEntityAt(size_t denseIndex) const {
        assert(denseIndex < m_denseToEntity.size());
        return m_denseToEntity[denseIndex];
    }

    // Get all entities that have this component
    const std::vector<EntityID>& entities() const { return m_denseToEntity; }

    // Direct access to dense array for batch processing
    std::vector<T>& data() { return m_dense; }
    const std::vector<T>& data() const { return m_dense; }

private:
    static constexpr uint32_t INVALID_INDEX = std::numeric_limits<uint32_t>::max();

    std::vector<T> m_dense;              // Dense array of components (cache-friendly)
    std::vector<EntityID> m_denseToEntity; // Maps dense index -> entity ID
    std::vector<uint32_t> m_sparse;      // Maps entity index -> dense index
};

} // namespace ecs
