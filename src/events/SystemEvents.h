#pragma once

#include "Event.h"
#include <string>

namespace events {

// Application lifecycle events
struct ApplicationStartedEvent : public EventBase<ApplicationStartedEvent> {
    ApplicationStartedEvent() = default;
};

struct ApplicationShutdownRequestedEvent : public EventBase<ApplicationShutdownRequestedEvent> {
    ApplicationShutdownRequestedEvent() = default;
};

struct FrameStartEvent : public EventBase<FrameStartEvent> {
    float deltaTime;
    float totalTime;
    uint64_t frameNumber;

    FrameStartEvent(float dt, float tt, uint64_t fn)
        : deltaTime(dt), totalTime(tt), frameNumber(fn) {}
};

struct FrameEndEvent : public EventBase<FrameEndEvent> {
    float frameTimeMs;

    explicit FrameEndEvent(float ft) : frameTimeMs(ft) {}
};

// Scene lifecycle events
struct SceneLoadingEvent : public EventBase<SceneLoadingEvent> {
    std::string sceneName;

    explicit SceneLoadingEvent(const std::string& name) : sceneName(name) {}
};

struct SceneLoadedEvent : public EventBase<SceneLoadedEvent> {
    std::string sceneName;
    bool success;

    SceneLoadedEvent(const std::string& name, bool s) : sceneName(name), success(s) {}
};

struct SceneUnloadingEvent : public EventBase<SceneUnloadingEvent> {
    std::string sceneName;

    explicit SceneUnloadingEvent(const std::string& name) : sceneName(name) {}
};

// Resource events
struct ResourceLoadedEvent : public EventBase<ResourceLoadedEvent> {
    enum class ResourceType {
        Shader,
        Texture,
        Model,
        Audio
    };

    ResourceType type;
    std::string name;
    bool success;

    ResourceLoadedEvent(ResourceType t, const std::string& n, bool s)
        : type(t), name(n), success(s) {}
};

// Performance/stats events
struct PerformanceStatsEvent : public EventBase<PerformanceStatsEvent> {
    float fps;
    float frameTimeMs;
    int visibleObjects;
    int totalObjects;
    int drawCalls;
    size_t memoryUsageMb;

    PerformanceStatsEvent(float f, float ft, int vo, int to, int dc, size_t mem)
        : fps(f), frameTimeMs(ft), visibleObjects(vo), totalObjects(to),
          drawCalls(dc), memoryUsageMb(mem) {}
};

// Error/warning events
struct ErrorEvent : public EventBase<ErrorEvent> {
    enum class Severity {
        Info,
        Warning,
        Error,
        Critical
    };

    Severity severity;
    std::string source;
    std::string message;

    ErrorEvent(Severity s, const std::string& src, const std::string& msg)
        : severity(s), source(src), message(msg) {}
};

// Object system events
struct ObjectCountChangedEvent : public EventBase<ObjectCountChangedEvent> {
    int previousCount;
    int newCount;

    ObjectCountChangedEvent(int prev, int now) : previousCount(prev), newCount(now) {}
};

// Entity events (for ECS integration)
struct EntityCreatedEvent : public EventBase<EntityCreatedEvent> {
    uint32_t entityId;
    std::string entityType;

    EntityCreatedEvent(uint32_t id, const std::string& type)
        : entityId(id), entityType(type) {}
};

struct EntityDestroyedEvent : public EventBase<EntityDestroyedEvent> {
    uint32_t entityId;

    explicit EntityDestroyedEvent(uint32_t id) : entityId(id) {}
};

} // namespace events
