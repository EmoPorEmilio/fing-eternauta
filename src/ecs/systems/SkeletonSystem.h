#pragma once
#include "../Registry.h"

class SkeletonSystem {
public:
    void update(Registry& registry) {
        registry.forEachSkeleton([](Entity entity, Skeleton& skeleton) {
            if (skeleton.joints.empty()) return;

            std::vector<glm::mat4> globalTransforms(skeleton.joints.size());

            for (size_t i = 0; i < skeleton.joints.size(); ++i) {
                const auto& joint = skeleton.joints[i];

                if (joint.parentIndex >= 0) {
                    globalTransforms[i] = globalTransforms[joint.parentIndex] * joint.localTransform;
                } else {
                    globalTransforms[i] = joint.localTransform;
                }

                skeleton.boneMatrices[i] = globalTransforms[i] * joint.inverseBindMatrix;
            }
        });
    }
};
