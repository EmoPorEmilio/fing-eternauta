#pragma once
#include <glm/glm.hpp>
#include <vector>
#include <string>

struct Joint {
    int parentIndex = -1;
    glm::mat4 inverseBindMatrix{1.0f};
    glm::mat4 localTransform{1.0f};
};

struct Skeleton {
    std::vector<Joint> joints;
    std::vector<glm::mat4> boneMatrices;        // Skinning matrices (for GPU)
    std::vector<glm::mat4> jointWorldTransforms; // Actual world position of each joint
    std::vector<glm::mat4> bindPoseTransforms;  // Original pose to reset to
    std::vector<std::string> jointNames;        // Names from GLTF nodes

    void resize(size_t count) {
        joints.resize(count);
        boneMatrices.resize(count, glm::mat4(1.0f));
        jointWorldTransforms.resize(count, glm::mat4(1.0f));
        bindPoseTransforms.resize(count, glm::mat4(1.0f));
        jointNames.resize(count);
    }

    void resetToBindPose() {
        for (size_t i = 0; i < joints.size() && i < bindPoseTransforms.size(); ++i) {
            joints[i].localTransform = bindPoseTransforms[i];
        }
    }
};
