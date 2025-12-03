#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <vector>
#include <cstdint>

namespace ecs {

// Animation playback state
enum class AnimationState : uint8_t {
    STOPPED,
    PLAYING,
    PAUSED
};

// Animation wrap mode
enum class AnimationWrapMode : uint8_t {
    ONCE,       // Play once and stop
    LOOP,       // Loop indefinitely
    PING_PONG   // Play forward then backward
};

struct AnimationComponent {
    // Current animation clip index (-1 = no animation)
    int activeClipIndex = -1;

    // Playback state
    AnimationState state = AnimationState::STOPPED;
    AnimationWrapMode wrapMode = AnimationWrapMode::LOOP;

    // Timing
    float currentTime = 0.0f;
    float speed = 1.0f;
    float duration = 0.0f;

    // Blend weight (for future animation blending)
    float weight = 1.0f;

    // Direction for ping-pong mode
    bool playingForward = true;

    // Computed joint matrices (updated by AnimationSystem)
    // These are the final skinning matrices: globalTransform * inverseBindMatrix
    std::vector<glm::mat4> jointMatrices;

    // Number of joints (for validation)
    size_t jointCount = 0;

    AnimationComponent() = default;

    explicit AnimationComponent(int clipIndex, float clipDuration = 0.0f)
        : activeClipIndex(clipIndex), duration(clipDuration) {}

    // Convenience methods
    void play() {
        state = AnimationState::PLAYING;
    }

    void pause() {
        state = AnimationState::PAUSED;
    }

    void stop() {
        state = AnimationState::STOPPED;
        currentTime = 0.0f;
        playingForward = true;
    }

    void setClip(int clipIndex, float clipDuration) {
        activeClipIndex = clipIndex;
        duration = clipDuration;
        currentTime = 0.0f;
        playingForward = true;
    }

    bool isPlaying() const {
        return state == AnimationState::PLAYING;
    }

    // Advance time by deltaTime, respecting speed and wrap mode
    void advanceTime(float deltaTime) {
        if (state != AnimationState::PLAYING || duration <= 0.0f) return;

        float delta = deltaTime * speed * (playingForward ? 1.0f : -1.0f);
        currentTime += delta;

        switch (wrapMode) {
            case AnimationWrapMode::ONCE:
                if (currentTime >= duration) {
                    currentTime = duration;
                    state = AnimationState::STOPPED;
                } else if (currentTime < 0.0f) {
                    currentTime = 0.0f;
                    state = AnimationState::STOPPED;
                }
                break;

            case AnimationWrapMode::LOOP:
                while (currentTime >= duration) {
                    currentTime -= duration;
                }
                while (currentTime < 0.0f) {
                    currentTime += duration;
                }
                break;

            case AnimationWrapMode::PING_PONG:
                if (currentTime >= duration) {
                    currentTime = duration - (currentTime - duration);
                    playingForward = false;
                } else if (currentTime < 0.0f) {
                    currentTime = -currentTime;
                    playingForward = true;
                }
                break;
        }
    }

    // Get normalized time (0-1)
    float getNormalizedTime() const {
        if (duration <= 0.0f) return 0.0f;
        return currentTime / duration;
    }
};

} // namespace ecs
