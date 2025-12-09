#pragma once
#include "../Registry.h"

class SkeletonSystem {
public:
    void update(Registry& registry) {
        registry.forEachSkeleton([](Entity entity, Skeleton& skeleton) {
            if (skeleton.joints.empty()) return;

            // Ensure jointWorldTransforms is the right size
            if (skeleton.jointWorldTransforms.size() != skeleton.joints.size()) {
                skeleton.jointWorldTransforms.resize(skeleton.joints.size(), glm::mat4(1.0f));
            }

            for (size_t i = 0; i < skeleton.joints.size(); ++i) {
                const auto& joint = skeleton.joints[i];

                if (joint.parentIndex >= 0) {
                    skeleton.jointWorldTransforms[i] = skeleton.jointWorldTransforms[joint.parentIndex] * joint.localTransform;
                } else {
                    skeleton.jointWorldTransforms[i] = joint.localTransform;
                }

                // boneMatrices for GPU skinning (with inverseBindMatrix)
                skeleton.boneMatrices[i] = skeleton.jointWorldTransforms[i] * joint.inverseBindMatrix;
            }
        });
    }
};
