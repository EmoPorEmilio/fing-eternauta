#pragma once

#include "Event.h"
#include <glm/glm.hpp>

namespace events {

// Fog configuration changed
struct FogConfigChangedEvent : public EventBase<FogConfigChangedEvent> {
    bool enabled;
    glm::vec3 color;
    float density;
    float desaturationStrength;
    float absorptionDensity;
    float absorptionStrength;

    FogConfigChangedEvent(bool e, const glm::vec3& c, float d, float ds, float ad, float as)
        : enabled(e), color(c), density(d), desaturationStrength(ds),
          absorptionDensity(ad), absorptionStrength(as) {}
};

// Lighting configuration changed
struct LightingConfigChangedEvent : public EventBase<LightingConfigChangedEvent> {
    glm::vec3 ambientColor;
    float ambientIntensity;
    float specularStrength;
    float shininess;

    LightingConfigChangedEvent(const glm::vec3& ac, float ai, float ss, float sh)
        : ambientColor(ac), ambientIntensity(ai), specularStrength(ss), shininess(sh) {}
};

// Flashlight settings changed
struct FlashlightConfigChangedEvent : public EventBase<FlashlightConfigChangedEvent> {
    bool enabled;
    glm::vec3 color;
    float brightness;
    float cutoffDegrees;

    FlashlightConfigChangedEvent(bool e, const glm::vec3& c, float b, float cut)
        : enabled(e), color(c), brightness(b), cutoffDegrees(cut) {}
};

// Performance preset changed
struct PerformancePresetChangedEvent : public EventBase<PerformancePresetChangedEvent> {
    enum class Preset {
        Low,      // 10k objects
        Medium,   // 100k objects
        High,     // 250k objects
        Ultra,    // 500k objects
        Custom
    };

    Preset preset;
    int objectCount;
    bool frustumCullingEnabled;
    bool lodEnabled;

    PerformancePresetChangedEvent(Preset p, int count, bool fc, bool lod)
        : preset(p), objectCount(count), frustumCullingEnabled(fc), lodEnabled(lod) {}
};

// Snow system configuration changed
struct SnowConfigChangedEvent : public EventBase<SnowConfigChangedEvent> {
    bool enabled;
    int count;
    float fallSpeed;
    float windSpeed;
    float windDirection;
    float spriteSize;
    float timeScale;
    bool bulletCollisionEnabled;

    SnowConfigChangedEvent(bool e, int c, float fs, float ws, float wd, float ss, float ts, bool bc)
        : enabled(e), count(c), fallSpeed(fs), windSpeed(ws), windDirection(wd),
          spriteSize(ss), timeScale(ts), bulletCollisionEnabled(bc) {}
};

// Debug visualization changed
struct DebugConfigChangedEvent : public EventBase<DebugConfigChangedEvent> {
    bool showGrid;
    bool showOriginAxes;
    bool showNormals;
    bool wireframeMode;

    DebugConfigChangedEvent(bool grid, bool axes, bool normals, bool wireframe)
        : showGrid(grid), showOriginAxes(axes), showNormals(normals), wireframeMode(wireframe) {}
};

// Material configuration changed (ambient, specular, normal, roughness)
struct MaterialConfigChangedEvent : public EventBase<MaterialConfigChangedEvent> {
    float ambient;
    float specularStrength;
    float normalStrength;
    float roughnessBias;

    MaterialConfigChangedEvent(float a, float ss, float ns, float rb)
        : ambient(a), specularStrength(ss), normalStrength(ns), roughnessBias(rb) {}
};

// Debug visualization extended (grid scale, fade distance, gizmo, floor mode)
struct DebugVisualsChangedEvent : public EventBase<DebugVisualsChangedEvent> {
    bool showGrid;
    bool showOriginAxes;
    bool showGizmo;
    float gridScale;
    float gridFadeDistance;
    int floorMode;  // 0=GridOnly, 1=TexturedSnow, 2=Both

    DebugVisualsChangedEvent(bool sg, bool sa, bool sgm, float gs, float gfd, int fm)
        : showGrid(sg), showOriginAxes(sa), showGizmo(sgm),
          gridScale(gs), gridFadeDistance(gfd), floorMode(fm) {}
};

// Camera configuration changed
struct CameraConfigChangedEvent : public EventBase<CameraConfigChangedEvent> {
    float fov;
    float nearPlane;
    float farPlane;
    float moveSpeed;
    float mouseSensitivity;

    CameraConfigChangedEvent(float f, float n, float fa, float ms, float sens)
        : fov(f), nearPlane(n), farPlane(fa), moveSpeed(ms), mouseSensitivity(sens) {}
};

} // namespace events
