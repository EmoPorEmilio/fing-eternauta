#pragma once
#include "../Registry.h"
#include "../../assets/AssetLoader.h"
#include <glm/gtx/quaternion.hpp>
#include <cmath>

class AnimationSystem {
public:
    void update(Registry& registry, float dt) {
        registry.forEachAnimated([&](Entity entity, Animation& anim, Skeleton& skeleton) {
            if (!anim.playing) return;

            // Clips now live in the Animation component
            if (anim.clipIndex < 0 || anim.clipIndex >= static_cast<int>(anim.clips.size())) return;

            const AnimationClip& clip = anim.clips[anim.clipIndex];

            anim.time += dt * anim.speedMultiplier;
            if (clip.duration > 0.0f) {
                anim.time = fmod(anim.time, clip.duration);
            }

            for (const auto& channel : clip.channels) {
                if (channel.jointIndex < 0 || channel.jointIndex >= static_cast<int>(skeleton.joints.size())) continue;

                skeleton.joints[channel.jointIndex].localTransform = interpolateTransform(channel, anim.time);
            }
        });
    }

private:
    static bool findKeyframes(const std::vector<float>& times, float t, size_t& i0, size_t& i1, float& factor) {
        if (times.empty()) return false;
        if (times.size() == 1) {
            i0 = i1 = 0;
            factor = 0.0f;
            return true;
        }
        if (t <= times[0]) {
            i0 = i1 = 0;
            factor = 0.0f;
            return true;
        }
        for (size_t i = 0; i < times.size() - 1; ++i) {
            if (t >= times[i] && t <= times[i + 1]) {
                i0 = i;
                i1 = i + 1;
                float dt = times[i1] - times[i0];
                factor = (dt > 0.0f) ? (t - times[i0]) / dt : 0.0f;
                return true;
            }
        }
        i0 = i1 = times.size() - 1;
        factor = 0.0f;
        return true;
    }

    static glm::mat4 interpolateTransform(const AnimationChannel& channel, float time) {
        glm::vec3 translation(0.0f);
        glm::quat rotation(1.0f, 0.0f, 0.0f, 0.0f);
        glm::vec3 scale(1.0f);

        size_t i0, i1;
        float factor;

        if (!channel.translations.empty() && !channel.translationTimes.empty()) {
            if (findKeyframes(channel.translationTimes, time, i0, i1, factor)) {
                if (i0 < channel.translations.size() && i1 < channel.translations.size()) {
                    translation = glm::mix(channel.translations[i0], channel.translations[i1], factor);
                }
            }
        }

        if (!channel.rotations.empty() && !channel.rotationTimes.empty()) {
            if (findKeyframes(channel.rotationTimes, time, i0, i1, factor)) {
                if (i0 < channel.rotations.size() && i1 < channel.rotations.size()) {
                    rotation = glm::slerp(channel.rotations[i0], channel.rotations[i1], factor);
                }
            }
        }

        if (!channel.scales.empty() && !channel.scaleTimes.empty()) {
            if (findKeyframes(channel.scaleTimes, time, i0, i1, factor)) {
                if (i0 < channel.scales.size() && i1 < channel.scales.size()) {
                    scale = glm::mix(channel.scales[i0], channel.scales[i1], factor);
                }
            }
        }

        glm::mat4 T = glm::translate(glm::mat4(1.0f), translation);
        glm::mat4 R = glm::toMat4(rotation);
        glm::mat4 S = glm::scale(glm::mat4(1.0f), scale);

        return T * R * S;
    }
};
