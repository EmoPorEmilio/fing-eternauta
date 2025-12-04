#pragma once

#include "../Prism.h" // For LODLevel enum

namespace ecs {

struct LODComponent {
    // Current LOD level (updated by LODSystem)
    LODLevel currentLevel = LODLevel::HIGH;

    // Distance thresholds for LOD transitions
    float highDistance = 50.0f;    // Use HIGH LOD when distance < this
    float mediumDistance = 150.0f; // Use MEDIUM LOD when distance < this, else LOW

    // Cached distance to camera (updated by LODSystem)
    float distanceToCamera = 0.0f;

    // LOD bias - positive values force lower detail, negative force higher
    float lodBias = 0.0f;

    LODComponent() = default;

    LODComponent(float highDist, float medDist)
        : highDistance(highDist), mediumDistance(medDist) {}

    // Calculate LOD level from distance
    LODLevel calculateLOD(float distance) const {
        float adjustedDistance = distance + lodBias;

        if (adjustedDistance <= highDistance) {
            return LODLevel::HIGH;
        } else if (adjustedDistance <= mediumDistance) {
            return LODLevel::MEDIUM;
        } else {
            return LODLevel::LOW;
        }
    }

    // Update LOD based on current distance
    void updateLOD() {
        currentLevel = calculateLOD(distanceToCamera);
    }
};

} // namespace ecs
