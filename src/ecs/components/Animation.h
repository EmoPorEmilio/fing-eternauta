#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <string>

struct AnimationChannel {
    int jointIndex = -1;
    std::vector<float> translationTimes;
    std::vector<float> rotationTimes;
    std::vector<float> scaleTimes;
    std::vector<glm::vec3> translations;
    std::vector<glm::quat> rotations;
    std::vector<glm::vec3> scales;
};

struct AnimationClip {
    std::string name;
    float duration = 0.0f;
    std::vector<AnimationChannel> channels;
};

struct Animation {
    int clipIndex = 0;
    float time = 0.0f;
    bool playing = true;
};
