#pragma once
#include <glm/glm.hpp>
#include <vector>

struct Joint {
    int parentIndex = -1;
    glm::mat4 inverseBindMatrix{1.0f};
    glm::mat4 localTransform{1.0f};
};

struct Skeleton {
    std::vector<Joint> joints;
    std::vector<glm::mat4> boneMatrices;
    std::vector<glm::mat4> bindPoseTransforms;  // Original pose to reset to

    void resize(size_t count) {
        joints.resize(count);
        boneMatrices.resize(count, glm::mat4(1.0f));
        bindPoseTransforms.resize(count, glm::mat4(1.0f));
    }

    void resetToBindPose() {
        for (size_t i = 0; i < joints.size() && i < bindPoseTransforms.size(); ++i) {
            joints[i].localTransform = bindPoseTransforms[i];
        }
    }
};
