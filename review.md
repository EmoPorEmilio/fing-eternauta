# Graphics Architecture Review

**Reviewer perspective:** 20 years GPU/graphics engineering experience
**Project:** fing-eternauta-from-scratch (OpenGL 4.5 / SDL3 game engine)

---

## CRITICAL ISSUES

### 1. No Render State Management

OpenGL state calls (`glEnable`/`glDisable`/`glBlendFunc`) are scattered throughout the codebase without tracking. Every frame potentially sets the same GL state dozens of times redundantly.

```cpp
// RenderHelpers.h:93-96 - called every frame even if already set
glDisable(GL_DEPTH_TEST);
glEnable(GL_BLEND);
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

**Impact:** Unnecessary driver overhead on every frame.

**Fix:** Create a `RenderState` class that caches current GL state and only issues calls when state actually changes.

---

### 2. Shader Uniform Abuse

Uniforms are set every single frame via `setMat4`, `setVec3`, `setInt` even for values that rarely or never change. This is death by a thousand cuts on the driver.

```cpp
// RenderPipeline.h:92-101 - every frame, every uniform
m_renderConfig.groundShader->setMat4("uView", view);
m_renderConfig.groundShader->setMat4("uProjection", projection);
m_renderConfig.groundShader->setMat4("uModel", glm::mat4(1.0f)); // ALWAYS IDENTITY!
```

**Impact:** Hundreds of unnecessary uniform updates per frame.

**Fix:** Use Uniform Buffer Objects (UBOs) for shared data (view, projection, light). Only update what actually changes.

---

### 3. String Lookups for Uniforms

Every `setMat4("uView", ...)` performs a string hash lookup internally. This happens hundreds of times per frame.

```cpp
// Shader.h (typical implementation)
void setMat4(const std::string& name, const glm::mat4& value) {
    glUniformMatrix4fv(glGetUniformLocation(m_program, name.c_str()), ...);
}
```

**Impact:** CPU overhead from repeated string hashing and GL queries.

**Fix:** Cache uniform locations at shader load time:
```cpp
GLint viewLoc = glGetUniformLocation(program, "uView");
// Then use: glUniformMatrix4fv(viewLoc, ...)
```

---

### 4. No Batching or Instancing

Buildings are rendered one draw call at a time:

```cpp
// RenderPipeline.h:57-68
for (Entity e : shadowCasters) {
    m_shadowConfig.depthShader->setMat4("uModel", t->matrix());
    // Draw one building...
}
```

With hundreds of identical cubes sharing the same mesh, this should use **instanced rendering** (`glDrawElementsInstanced`). One draw call instead of hundreds.

**Impact:** Draw call bound - GPU starves waiting for CPU to submit commands.

**Fix:** Implement instanced rendering for buildings. Store transforms in a buffer, draw all instances in one call.

---

### 5. Shadow Map Architecture is Naive

Current implementation has several limitations:

- Fixed orthographic size regardless of scene content
- Single cascade shadow map (no CSM)
- No depth bias adjustment based on surface angle
- Shadow frustum doesn't properly follow camera for directional lights

```cpp
// RenderHelpers.h:17-24
float orthoSize = GameConfig::SHADOW_ORTHO_SIZE;  // Fixed size
glm::mat4 lightProjection = glm::ortho(-orthoSize, orthoSize, ...);
```

**Impact:** Shadow quality degrades at distance, shadow acne, peter-panning artifacts.

**Fix:** For a city-scale scene, implement Cascaded Shadow Maps (CSM) at minimum. Add slope-scaled depth bias.

---

## STRUCTURAL PROBLEMS

### 6. RenderHelpers is a Code Smell

`inline` functions in a header with `namespace RenderHelpers` is essentially C with namespaces. This isn't encapsulation - it's organized chaos.

```cpp
namespace RenderHelpers {
    inline void renderShadowPass(...) { /* 20 lines */ }
    inline void renderGroundPlane(...) { /* 25 lines */ }
    // etc.
}
```

Every translation unit that includes this header recompiles when anything changes. The `RenderPipeline` class exists but isn't used - two competing patterns for the same responsibility.

**Fix:** Consolidate into `RenderPipeline` class. Move implementations to .cpp file. Delete `RenderHelpers.h`.

---

### 7. SceneContext is a God Object

```cpp
struct SceneContext {
    Registry& registry;
    SceneManager& sceneManager;
    GameState& gameState;
    InputSystem& inputSystem;
    RenderSystem& renderSystem;
    UISystem& uiSystem;
    // ... 30+ more members
};
```

This passes everything to everything. Scenes should request specific services, not receive the entire engine.

**Impact:** No clear dependencies, impossible to test in isolation, coupling nightmare.

**Fix:** Use dependency injection. Scenes declare what they need. A service locator or explicit constructor injection provides only required services.

---

### 8. Entity Pool Pattern is Wrong

```cpp
// RenderHelpers.h:144-148
for (; entityIdx < buildingEntityPool.size(); ++entityIdx) {
    transform->position.y = -1000.0f; // "Hide" by moving underground
}
```

Entities are "hidden" by moving them to Y=-1000. They still exist in every system iteration, still processed by transforms, still checked for rendering.

**Impact:** Wasted CPU cycles processing "invisible" entities.

**Fix:** Add a visibility/enabled component flag. Systems skip disabled entities entirely.

---

### 9. LOD System is Primitive

```cpp
// RenderHelpers.h:162-170
float dist = glm::length(viewerPos - fingT->position);
bool shouldUseHighDetail = dist < lodSwitchDistance;
```

Single distance threshold with instant pop. No hysteresis (causes flickering at boundary), no blending, no consideration of screen-space size.

**Impact:** Visible pop when crossing LOD boundary. Flickering if player hovers near threshold.

**Fix:** Add hysteresis (different thresholds for switch-up vs switch-down). Consider screen-space projected size, not world distance.

---

## PERFORMANCE RED FLAGS

### 10. Texture Binding Pattern

```cpp
glActiveTexture(GL_TEXTURE0);
glBindTexture(GL_TEXTURE_2D, snowTexture);
groundShader.setInt("uTexture", 0);
glActiveTexture(GL_TEXTURE1);
glBindTexture(GL_TEXTURE_2D, shadowDepthTexture);
```

Using OpenGL 4.5 but writing OpenGL 2.1 patterns.

**Fix:** Use DSA (Direct State Access): `glBindTextureUnit(0, snowTexture)`. Or bindless textures for maximum performance.

---

### 11. No Frustum Culling

Building culling is grid-based only:

```cpp
// RenderHelpers.h:127-131
if (BuildingGenerator::isBuildingInRange(bldg, playerGridX, playerGridZ, buildingRenderRadius)) {
    visibleBuildings.push_back(&bldg);
}
```

- No view frustum culling
- No occlusion culling
- Buildings behind camera still process through shadow pass

**Impact:** Rendering objects that aren't visible.

**Fix:** Implement frustum culling before any render pass. Consider occlusion culling for dense city scenes.

---

### 12. Magic Numbers

```cpp
if (t && t->position.y > -100.0f)   // What is -100?
transform->position.y = -1000.0f;    // What is -1000?
```

Hardcoded thresholds scattered throughout code.

**Fix:** Move to `GameConfig.h` as named constants:
```cpp
static constexpr float ENTITY_HIDE_Y = -1000.0f;
static constexpr float ENTITY_VISIBLE_THRESHOLD = -100.0f;
```

---

## WHAT'S DONE RIGHT

- **ECS architecture direction** - Separating concerns is correct
- **FBO usage** - Shadow mapping and post-processing use framebuffers correctly
- **Triplanar mapping** - Smart choice for procedural buildings
- **Normal mapping** - Proper tangent-space implementation
- **Scene system abstraction** - Right pattern despite SceneContext bloat
- **Ping-pong buffers** - Motion blur shows understanding of post-processing
- **LOD concept** - Having LOD at all is good, implementation needs work

---

## PRIORITY RECOMMENDATIONS

| Priority | Task | Impact |
|----------|------|--------|
| 1 | Implement UBOs for shared uniforms | Medium perf gain, cleaner code |
| 2 | Add instanced rendering for buildings | 10x+ draw call reduction |
| 3 | Cache uniform locations | CPU overhead reduction |
| 4 | Implement frustum culling | Stop rendering invisible objects |
| 5 | Fix entity visibility (component flag) | Stop processing hidden entities |
| 6 | Consolidate RenderHelpers → RenderPipeline | Code maintainability |
| 7 | Add render state caching | Reduce redundant GL calls |
| 8 | Add LOD hysteresis | Eliminate LOD pop/flicker |

---

## SUMMARY

The architecture shows understanding of graphics concepts but implementation is at student-project level. The bones are solid - ECS, scene management, shadow mapping, post-processing are all present. The execution needs professional-grade optimization: batching, state management, proper culling, and cleaner abstractions.

Current state: **Functional prototype**
Target state: **Production-ready engine**

The gap is primarily performance optimization and code organization, not fundamental architecture changes.
