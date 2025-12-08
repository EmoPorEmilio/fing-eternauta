#pragma once
#include "../ecs/components/Mesh.h"
#include "../ecs/components/Skeleton.h"
#include "../ecs/components/Animation.h"
#include <string>
#include <vector>
#include <optional>

struct LoadedModel {
    MeshGroup meshGroup;
    std::optional<Skeleton> skeleton;
    std::vector<AnimationClip> clips;
    std::vector<GLuint> textures;
};

LoadedModel loadGLB(const std::string& path);
