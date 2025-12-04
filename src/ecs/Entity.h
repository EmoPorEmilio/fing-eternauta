#pragma once

#include <cstdint>
#include <limits>

namespace ecs {

// Entity ID type - 32 bits split into index (20 bits) and generation (12 bits)
// This allows ~1M entities and 4096 generations before wraparound
using EntityID = uint32_t;

constexpr EntityID INVALID_ENTITY = std::numeric_limits<EntityID>::max();

// Extract index from entity ID (lower 20 bits)
inline constexpr uint32_t getEntityIndex(EntityID id) {
    return id & 0xFFFFF;
}

// Extract generation from entity ID (upper 12 bits)
inline constexpr uint16_t getEntityGeneration(EntityID id) {
    return static_cast<uint16_t>((id >> 20) & 0xFFF);
}

// Create entity ID from index and generation
inline constexpr EntityID makeEntityID(uint32_t index, uint16_t generation) {
    return (static_cast<EntityID>(generation & 0xFFF) << 20) | (index & 0xFFFFF);
}

// Entity handle with generation for safe references
struct Entity {
    EntityID id = INVALID_ENTITY;

    Entity() = default;
    explicit Entity(EntityID entityId) : id(entityId) {}

    bool isValid() const { return id != INVALID_ENTITY; }
    uint32_t index() const { return getEntityIndex(id); }
    uint16_t generation() const { return getEntityGeneration(id); }

    bool operator==(const Entity& other) const { return id == other.id; }
    bool operator!=(const Entity& other) const { return id != other.id; }
    bool operator<(const Entity& other) const { return id < other.id; }
};

} // namespace ecs
