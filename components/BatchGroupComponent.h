#pragma once

#include "../Prism.h" // For LODLevel enum
#include <cstdint>

namespace ecs {

// Batch IDs for grouping entities for GPU instancing
enum class BatchID : uint32_t {
    PRISM = 0,    // Instanced prisms
    SNOW = 1,     // Snow particles
    // Add more batch types as needed
};

struct BatchGroupComponent {
    // Which batch this entity belongs to
    BatchID batchId = BatchID::PRISM;

    // LOD level for this batch (affects which VAO to use)
    LODLevel lodLevel = LODLevel::HIGH;

    // Index within the batch's instance buffer (set by render system)
    uint32_t instanceIndex = 0;

    // Whether this entity should be included in next batch update
    bool batchDirty = true;

    BatchGroupComponent() = default;

    explicit BatchGroupComponent(BatchID batch)
        : batchId(batch) {}

    BatchGroupComponent(BatchID batch, LODLevel lod)
        : batchId(batch), lodLevel(lod) {}
};

} // namespace ecs
