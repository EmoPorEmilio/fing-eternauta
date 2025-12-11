#pragma once
#include "../ecs/components/Mesh.h"
#include "../ecs/components/Skeleton.h"
#include "../ecs/components/Animation.h"
#include <glm/glm.hpp>
#include <string>
#include <vector>
#include <optional>
#include <cfloat>

// Axis-aligned bounding box computed from model vertices
struct ModelBounds {
    glm::vec3 min{FLT_MAX, FLT_MAX, FLT_MAX};
    glm::vec3 max{-FLT_MAX, -FLT_MAX, -FLT_MAX};

    bool isValid() const { return min.x <= max.x; }
    glm::vec3 center() const { return (min + max) * 0.5f; }
    glm::vec3 size() const { return max - min; }
    glm::vec3 halfExtents() const { return size() * 0.5f; }
};

struct LoadedModel {
    MeshGroup meshGroup;
    std::optional<Skeleton> skeleton;
    std::vector<AnimationClip> clips;
    std::vector<GLuint> textures;
    ModelBounds bounds;  // AABB computed from all mesh vertices
};

LoadedModel loadGLB(const std::string& path);
