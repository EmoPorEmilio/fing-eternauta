# Refactoring Proposal

This document outlines recommended improvements to the OpenGL graphics engine, prioritized by impact and effort.

## Priority 1: Uniform Location Caching

### Problem

Every frame, `glGetUniformLocation()` is called 50+ times across all managers:

```cpp
// ObjectManager::render() - called every frame
GLint viewLoc = glGetUniformLocation(shaderProgram, "uView");
GLint projLoc = glGetUniformLocation(shaderProgram, "uProj");
GLint fogEnabledLoc = glGetUniformLocation(shaderProgram, "uFogEnabled");
// ... 20+ more calls
```

### Impact

- **CPU overhead**: String hashing and lookup per uniform per frame
- **Measurable in profiler**: ~0.1-0.5ms per frame on 50+ calls
- **Scales with light count**: LightManager uses string concatenation in loop

### Solution

Cache uniform locations after shader compilation:

```cpp
// Add to each manager class
struct UniformCache {
    GLint view = -1;
    GLint projection = -1;
    GLint model = -1;
    GLint fogEnabled = -1;
    GLint fogColor = -1;
    GLint fogDensity = -1;
    GLint fogDesaturationStrength = -1;
    GLint fogAbsorptionDensity = -1;
    GLint fogAbsorptionStrength = -1;
    GLint flashlightOn = -1;
    GLint flashlightPos = -1;
    GLint flashlightDir = -1;
    GLint flashlightCutoff = -1;
    GLint flashlightBrightness = -1;
    GLint flashlightColor = -1;
};

UniformCache m_uniforms;

// Call once after shader compilation
void cacheUniformLocations(GLuint program) {
    m_uniforms.view = glGetUniformLocation(program, "uView");
    m_uniforms.projection = glGetUniformLocation(program, "uProj");
    // ... etc
}

// Use cached locations in render
void render(...) {
    glUniformMatrix4fv(m_uniforms.view, 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(m_uniforms.projection, 1, GL_FALSE, glm::value_ptr(proj));
    glUniform1i(m_uniforms.fogEnabled, m_fogEnabled);
    // ... etc
}
```

### Files to Modify

| File | Changes |
|------|---------|
| ObjectManager.h | Add `UniformCache m_uniforms` |
| ObjectManager.cpp | Add `cacheUniformLocations()`, update `render()` |
| ModelManager.h | Add `UniformCache m_uniforms` |
| ModelManager.cpp | Add `cacheUniformLocations()`, update `render()` |
| SnowSystem.h | Add `UniformCache m_uniforms` |
| SnowSystem.cpp | Add `cacheUniformLocations()`, update `render()` |
| LightManager.cpp | Cache light array uniform locations |

### Effort

**Low** (1-2 hours)

---

## Priority 2: Consolidate Fog Parameters

### Problem

Fog settings are duplicated across 5+ classes:

```cpp
// In ObjectManager.h
bool fogEnabled;
glm::vec3 fogColor;
float fogDensity;
float fogDesaturationStrength;
float fogAbsorptionDensity;
float fogAbsorptionStrength;

// Same fields duplicated in:
// - ModelManager.h
// - SnowSystem.h
// - BaseScene.h
// - UIManager.h
```

### Impact

- **Sync bugs**: Values can get out of sync between classes
- **Update complexity**: Must update 5 places when changing fog
- **Memory waste**: 6 floats * 5 classes = 30 unnecessary floats

### Solution

Query ConfigManager directly in render methods:

```cpp
// Before (in each manager)
void render(...) {
    shader.setUniform("uFogEnabled", m_fogEnabled);
    shader.setUniform("uFogColor", m_fogColor);
    // ... using local copies
}

// After
void render(...) {
    const auto& fog = ConfigManager::instance().getFog();
    shader.setUniform("uFogEnabled", fog.enabled);
    shader.setUniform("uFogColor", fog.color);
    shader.setUniform("uFogDensity", fog.density);
    shader.setUniform("uFogDesaturationStrength", fog.desaturationStrength);
    shader.setUniform("uFogAbsorptionDensity", fog.absorptionDensity);
    shader.setUniform("uFogAbsorptionStrength", fog.absorptionStrength);
}
```

Or pass fog config as parameter:

```cpp
void render(..., const ConfigManager::FogConfig& fog) {
    // Use fog parameter directly
}
```

### Files to Modify

| File | Changes |
|------|---------|
| ObjectManager.h | Remove fog member variables |
| ObjectManager.cpp | Query ConfigManager in render() |
| ModelManager.h | Remove fog member variables |
| ModelManager.cpp | Query ConfigManager in render() |
| SnowSystem.h | Remove fog member variables |
| SnowSystem.cpp | Query ConfigManager in render() |
| BaseScene.cpp | Query ConfigManager in renderFloor() |

### Effort

**Low** (1-2 hours)

---

## Priority 3: Remove UI State Duplication

### Problem

UIManager maintains copies of ConfigManager state:

```cpp
// UIManager.h - duplicates ConfigManager values
float m_fogDensity = 0.0050f;
float m_fogColor[3] = {0.0667f, 0.0784f, 0.0980f};
bool m_fogEnabled = true;
// ... 30+ more duplicate fields
```

### Impact

- **Two sources of truth**: UIManager and ConfigManager can disagree
- **Sync complexity**: Must manually sync values
- **Bug-prone**: Easy to forget updating both places

### Solution

UIManager should read from ConfigManager for display and write via setters:

```cpp
// Before
void UIManager::renderFogPanel() {
    if (ImGui::Checkbox("Enabled", &m_fogEnabled)) {
        ConfigManager::instance().setFogEnabled(m_fogEnabled);
    }
    if (ImGui::SliderFloat("Density", &m_fogDensity, 0.0f, 0.1f)) {
        ConfigManager::instance().setFogDensity(m_fogDensity);
    }
}

// After
void UIManager::renderFogPanel() {
    auto fog = ConfigManager::instance().getFog();  // Get current values

    bool enabled = fog.enabled;
    if (ImGui::Checkbox("Enabled", &enabled)) {
        ConfigManager::instance().setFogEnabled(enabled);
    }

    float density = fog.density;
    if (ImGui::SliderFloat("Density", &density, 0.0f, 0.1f)) {
        ConfigManager::instance().setFogDensity(density);
    }
}
```

### Files to Modify

| File | Changes |
|------|---------|
| UIManager.h | Remove duplicate m_fog*, m_snow*, m_flashlight*, etc. |
| UIManager.cpp | Read from ConfigManager in render panels |

### Effort

**Medium** (2-3 hours)

---

## Priority 4: Migrate to ECS-Only Storage

### Problem

Managers maintain parallel data structures:

```cpp
// LightManager.h
std::vector<Light> lights;                    // Traditional storage
std::vector<ecs::Entity> m_lightEntities;     // ECS entities

// ObjectManager.h
std::vector<ecs::Entity> m_entities;          // ECS entities
// But also caches data in local vectors during render
```

### Impact

- **Memory duplication**: Same data in two places
- **Sync complexity**: Must keep vector and ECS in sync
- **Transition debt**: Partially migrated architecture

### Solution

Use ECS as the sole data store. Access via registry queries:

```cpp
// Before
void LightManager::addLight(const Light& light) {
    lights.push_back(light);                           // Old way
    m_lightEntities.push_back(createLightEntity(light)); // ECS way
}

// After
void LightManager::addLight(const Light& light) {
    auto entity = createLightEntity(light);  // Only ECS
    // No separate vector needed
}

// Before
const std::vector<Light>& getLights() const { return lights; }

// After
void forEachLight(std::function<void(Entity, const LightComponent&)> fn) {
    auto& reg = ECSWorld::getRegistry();
    reg.each<LightComponent>([&](Entity e, LightComponent& lc) {
        fn(e, lc);
    });
}
```

### Migration Steps

1. **Phase 1**: Ensure all access goes through ECS queries
2. **Phase 2**: Remove vector storage
3. **Phase 3**: Optimize ECS queries with views/groups

### Files to Modify

| File | Changes |
|------|---------|
| LightManager.h | Remove `std::vector<Light> lights` |
| LightManager.cpp | Replace vector access with ECS queries |
| ObjectManager.h | Keep entity vector for cleanup only |
| ObjectManager.cpp | Use ECS queries instead of gather functions |
| ModelManager.h | Remove model vector, use ECS only |
| SnowSystem.cpp | Use ECS queries for particle data |

### Effort

**High** (1-2 days)

---

## Priority 5: Instance Buffer Optimization

### Problem

Every frame, instance data is rebuilt and uploaded:

```cpp
// ObjectManager::render()
std::vector<glm::mat4> highInstances, mediumInstances, lowInstances;
gatherInstanceMatrices(highInstances, mediumInstances, lowInstances);  // CPU copy

glBindBuffer(GL_ARRAY_BUFFER, highLodInstanceVbo);
glBufferSubData(GL_ARRAY_BUFFER, 0,
    highInstances.size() * sizeof(glm::mat4),
    highInstances.data());  // GPU upload
// Repeat for medium and low...
```

### Impact

- **CPU-GPU bandwidth**: 500k * 64 bytes = 32MB/frame potential
- **CPU overhead**: Vector allocation and copy
- **Driver stalls**: Possible sync with in-flight GPU work

### Solution Options

#### Option A: Persistent Mapped Buffers (Recommended)

```cpp
// Initialization
glBindBuffer(GL_ARRAY_BUFFER, instanceVbo);
glBufferStorage(GL_ARRAY_BUFFER,
    maxInstances * sizeof(glm::mat4),
    nullptr,
    GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

m_mappedBuffer = (glm::mat4*)glMapBufferRange(GL_ARRAY_BUFFER, 0,
    maxInstances * sizeof(glm::mat4),
    GL_MAP_WRITE_BIT | GL_MAP_PERSISTENT_BIT | GL_MAP_COHERENT_BIT);

// Update (no explicit upload needed)
void updateInstances() {
    int idx = 0;
    registry.each<TransformComponent, RenderableComponent>([&](auto e, auto& t, auto& r) {
        if (r.visible) {
            m_mappedBuffer[idx++] = t.modelMatrix;
        }
    });
    m_visibleCount = idx;
}
```

#### Option B: Triple Buffering

Use 3 instance buffers and rotate:
- Frame N: GPU reads buffer 0
- Frame N: CPU writes buffer 1
- Frame N+1: GPU reads buffer 1
- Frame N+1: CPU writes buffer 2
- etc.

#### Option C: Compute Shader Updates

Move transform updates to GPU:

```glsl
#version 450
layout(local_size_x = 256) in;

layout(std430, binding = 0) buffer Positions {
    vec4 positions[];
};

layout(std430, binding = 1) buffer Matrices {
    mat4 matrices[];
};

void main() {
    uint id = gl_GlobalInvocationID.x;
    vec3 pos = positions[id].xyz;
    // Build matrix on GPU
    matrices[id] = mat4(...);
}
```

### Files to Modify

| File | Changes |
|------|---------|
| ObjectManager.h | Add mapped buffer pointers, remove gather vectors |
| ObjectManager.cpp | Use persistent mapping or triple buffering |
| SnowSystem.h | Same changes |
| SnowSystem.cpp | Same changes |

### Effort

**High** (2-3 days)

---

## Summary

| Priority | Issue | Impact | Effort | Recommendation |
|----------|-------|--------|--------|----------------|
| 1 | Uniform caching | High | Low | Do first |
| 2 | Fog consolidation | Medium | Low | Do second |
| 3 | UI state duplication | Medium | Medium | Do with fog |
| 4 | ECS-only storage | Low | High | Plan for future |
| 5 | Instance buffer | High | High | Profile first |

## Recommended Order

1. **Quick wins** (Priority 1 + 2): ~3 hours
   - Cache uniform locations
   - Consolidate fog parameters

2. **Architecture cleanup** (Priority 3): ~2 hours
   - Remove UIManager state duplication

3. **Performance deep-dive** (Priority 5): ~3 days
   - Profile current performance
   - Implement persistent mapped buffers
   - Measure improvement

4. **Long-term refactor** (Priority 4): ~2 days
   - Migrate to ECS-only when adding new features

## Testing Strategy

After each refactor:

1. **Visual check**: Ensure rendering looks identical
2. **Performance check**: Compare frame times before/after
3. **Stress test**: Run with PRESET_MAXIMUM (500k objects)
4. **UI check**: Ensure all settings still work
