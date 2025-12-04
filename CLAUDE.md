# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

This is a Visual Studio 2022 C++ project (Windows x64, v143 toolset).

**Build via MSBuild:**
```cmd
msbuild fing-eternauta.sln /p:Configuration=Debug /p:Platform=x64
msbuild fing-eternauta.sln /p:Configuration=Release /p:Platform=x64
```

**Run:** `x64/Debug/fing-eternauta.exe` or `x64/Release/fing-eternauta.exe`

The executable must be run from the repository root so shaders and assets are found.

## Architecture Overview

This is a modern OpenGL 4.5 graphics engine featuring PBR rendering, GPU-instanced rendering (500k+ objects), real-time snow simulation with Bullet3 physics, and atmospheric fog effects.

### Core Components

| Component | Responsibility |
|-----------|---------------|
| **Application** | Main loop, ImGui UI, event dispatch |
| **Renderer** | SDL2 window, OpenGL context, framebuffers |
| **Scene** | Orchestrates ObjectManager, ModelManager, SnowManager |
| **Camera** | FPS-style controls (WASD + mouse look) |
| **LightManager** | Multi-light system + flashlight with UBO (binding point 2) |
| **ObjectManager** | GPU-instanced prisms with 3-level LOD and frustum culling |
| **ModelManager** | GLTF model loading via tinygltf, instance transforms |
| **SnowManager** | Particle simulation with wind, optional Bullet3 collision |

### Shader Pipeline

- `phong_notess.vert/frag` - Floor with normal mapping
- `pbr_model.vert/frag` - GLTF models with PBR + flashlight + fog
- `object_instanced.vert/frag` - Instanced geometry rendering
- `snow_glow.vert/frag` - Billboard snow particles

All shaders share fog uniforms (`uFogEnabled`, `uFogColor`, `uFogDensity`, `uFogDesaturationStrength`).

### Key Patterns

- **Manager pattern**: Each subsystem (lights, objects, models, snow) encapsulated in its own manager class
- **LOD system**: HIGH (<50m), MEDIUM (50-150m), LOW (>150m) with per-LOD instance buffers
- **Linear color pipeline**: sRGB framebuffer, albedo textures as sRGB, metallic/roughness/height as linear
- **Centralized config**: All magic numbers in `Constants.h` organized by subsystem

## Naming Conventions

- Classes: `PascalCase`
- Private members: `m_camelCase`
- File-scope constants: `SCREAMING_SNAKE_CASE`
- Namespace constants: `Constants::Subsystem::VALUE`

## Directory Structure

```
src/              All source code (.h/.cpp files)
  ├── ecs/        Entity-Component-System core (Entity, Registry, System)
  ├── components/ ECS component definitions
  ├── systems/    ECS system implementations
  └── events/     Event types and EventBus
assets/           GLTF models, textures, fonts
shaders/          GLSL vertex/fragment shaders
libraries/        SDL2, GLM, GLAD, tinygltf, ImGui, Bullet3
tests/            Unit tests (math, components)
docs/             Architecture documentation
fing-eternauta/   VS project files
main.cpp          Application entry point (in root)
```

## Dependencies

SDL2 2.0.12, GLAD 4.5, GLM, tinygltf (includes stb_image), ImGui, Bullet3 (optional for snow physics). All bundled in `libraries/`.

## ECS Architecture

The engine uses a custom Entity-Component-System located in `src/ecs/` and `src/components/`.

### Entity IDs

Entities use 32-bit generational IDs to prevent ABA bugs:
- **Lower 20 bits**: Entity index (~1M max entities)
- **Upper 12 bits**: Generation counter (4096 generations per slot)

```cpp
// Entity creation and component attachment
auto entity = registry.create();
registry.add<TransformComponent>(entity, position, rotation, scale);
registry.add<RenderableComponent>(entity, RenderableType::INSTANCED_PRISM);

// Component access
auto& transform = registry.get<TransformComponent>(entity);
if (registry.has<LODComponent>(entity)) { /* ... */ }

// Iteration
registry.each<TransformComponent, RenderableComponent>([](Entity e, auto& t, auto& r) {
    // Process entities with both components
});
```

### Components (`src/components/`)

| Component | Key Fields | Purpose |
|-----------|------------|---------|
| **TransformComponent** | position, rotation, scale, modelMatrix | World transform with cached matrix |
| **RenderableComponent** | type, vao, shaderProgram, visible | Rendering metadata |
| **LODComponent** | currentLevel, highDistance, mediumDistance | LOD state and thresholds |
| **MaterialComponent** | baseColor, metallic, roughness, textures | PBR material properties |
| **LightComponent** | type, color, intensity, attenuation | Light source parameters |
| **ParticleComponent** | velocity, lifetime, age, seed | Particle physics state |
| **PhysicsComponent** | rigidBody, mass, friction | Bullet3 physics handles |

### Systems (`src/systems/`)

| System | Update Frequency | Responsibility |
|--------|-----------------|----------------|
| **LODSystem** | Every frame | Updates `LODComponent.currentLevel` from camera distance |
| **CullingSystem** | Every frame | Updates `RenderableComponent.visible` from distance |
| **RenderSystem** | Render phase | Gathers visible entities, batches by LOD |
| **TransformSystem** | On dirty | Recalculates model matrices |
| **ParticleSystem** | Every frame | Updates particle positions and lifetimes |

### ECSWorld Singleton

```cpp
// Access the global registry
auto& registry = ecs::ECSWorld::getRegistry();

// Access systems
auto* lodSystem = ecs::ECSWorld::getSystem<ecs::LODSystem>();
lodSystem->setCameraPosition(cameraPos);
```

---

## Event System

Events provide decoupled communication via `src/events/EventBus.h`.

### Usage

```cpp
// Subscribe to events (returns SubscriptionId for cleanup)
auto id = events::EventBus::instance().subscribe<FogConfigChangedEvent>(
    [this](const FogConfigChangedEvent& e) {
        m_fogEnabled = e.enabled;
        m_fogDensity = e.density;
    });

// Or with member function pointer
auto id = events::EventBus::instance().subscribe<FogConfigChangedEvent>(
    this, &MyClass::onFogConfigChanged);

// Publish events
events::EventBus::instance().publish(FogConfigChangedEvent{
    .enabled = true,
    .color = glm::vec3(0.1f),
    .density = 0.005f
});

// Cleanup (typically in destructor)
events::EventBus::instance().unsubscribe(id);
```

### Event Types (`src/events/`)

**Config Events** (`ConfigEvents.h`):
| Event | Triggered By | Fields |
|-------|--------------|--------|
| `FogConfigChangedEvent` | UIManager/ConfigManager | enabled, color, density, desaturation, absorption |
| `FlashlightConfigChangedEvent` | UIManager | enabled, color, brightness, cutoffDegrees |
| `PerformancePresetChangedEvent` | UIManager | preset, objectCount, culling, lod |
| `SnowConfigChangedEvent` | UIManager | enabled, count, fallSpeed, windSpeed, direction |
| `MaterialConfigChangedEvent` | UIManager | ambient, specular, normal, roughness |
| `DebugVisualsChangedEvent` | UIManager | grid, axes, gizmo, floorMode |
| `CameraConfigChangedEvent` | UIManager | fov, near, far, moveSpeed, sensitivity |

**Window Events** (`WindowEvents.h`):
- `WindowResizedEvent` - width, height
- `WindowClosedEvent` - (no fields)
- `WindowFocusEvent` - focused
- `WindowMinimizedEvent` - minimized

**Input Events** (`InputEvents.h`):
- `KeyPressedEvent`, `KeyReleasedEvent` - keycode, modifiers
- `MouseMovedEvent` - x, y, deltaX, deltaY
- `MouseButtonPressedEvent`, `MouseButtonReleasedEvent` - button

---

## Shader Conventions

### Shared Fog Uniforms

All primary fragment shaders implement consistent fog:

```glsl
uniform bool  uFogEnabled;
uniform vec3  uFogColor;
uniform float uFogDensity;              // Exponential fog factor
uniform float uFogDesaturationStrength; // Color desaturation (0-1)
uniform float uFogAbsorptionDensity;    // Light absorption rate
uniform float uFogAbsorptionStrength;   // Absorption blend (0-1)
uniform vec3  uBackgroundColor;         // Far fog target color
```

### Flashlight UBO (Binding Point 2)

```glsl
layout(std140, binding = 2) uniform FlashlightBlock {
    vec4 position;       // xyz = position, w = enabled (0/1)
    vec4 direction;      // xyz = direction, w = cutoff (cosine)
    vec4 colorBrightness;// xyz = color, w = brightness
    vec4 params;         // x = outerCutoff, yzw = reserved
};
```

### Shader Files

| Shader | GLSL Version | Instancing | Purpose |
|--------|--------------|------------|---------|
| `pbr_model.vert/frag` | 3.30 | No | GLTF models with PBR (GGX/Schlick) |
| `object_instanced.vert/frag` | 4.50 | Yes | 500k instanced prisms |
| `snow_glow.vert/frag` | 4.00 | Yes | Billboard particles |
| `phong_notess.vert/frag` | 4.50 | No | Textured floor with bump mapping |
| `debug_grid.vert/frag` | 4.50 | No | Infinite grid (procedural) |
| `debug_line.vert/frag` | 4.50 | No | Axes and gizmo lines |

---

## Manager Responsibilities

### LightManager
- **Owns**: Light array, flashlight state, flashlight UBO
- **GPU Resources**: UBO at binding point 2
- **Key Methods**: `addLight()`, `updateFlashlight()`, `applyLightsToShader()`
- **ECS Integration**: Creates entities with `LightComponent`

### ObjectManager
- **Owns**: Instanced prism geometry, LOD/culling systems
- **GPU Resources**: 3 VAOs (per LOD), 3 instance VBOs, shader program
- **Key Methods**: `initialize(count)`, `update(cameraPos)`, `render()`
- **Performance Presets**: MINIMAL=3k, MEDIUM=15k, MAXIMUM=500k

### ModelManager
- **Owns**: GLTF model storage, PBR shader
- **GPU Resources**: Per-model VAO/VBO/textures, shared PBR shader
- **Key Methods**: `loadModel()`, `addModelInstance()`, `render()`
- **Supports**: Skeletal animation, PBR materials, instancing

### SnowManager
- **Owns**: Particle simulation, impact puffs, optional Bullet3 world
- **GPU Resources**: Billboard VAO, instance VBO, puff instance VBO
- **Key Methods**: `initialize()`, `update(deltaTime)`, `render()`
- **Physics**: Optional ground collision via Bullet3 ray tests

### ConfigManager (Singleton)
- **Owns**: All runtime configuration (fog, lighting, performance, etc.)
- **Pattern**: Setters publish corresponding `ConfigChangedEvent`
- **Key Methods**: `getFog()`, `setFogEnabled()`, `getPerformance()`, etc.

### InputManager (Singleton)
- **Owns**: SDL event processing, keyboard/mouse state
- **Pattern**: Publishes input events to EventBus
- **Key Methods**: `processEvents()`, `isKeyDown()`, `getMouseDelta()`

### UIManager (Singleton)
- **Owns**: ImGui panel state
- **Pattern**: Reads from ConfigManager, writes via ConfigManager setters
- **Key Methods**: `renderSidebar()`, `renderMenuBar()`, panel renderers

---

## Performance Characteristics

### LOD Thresholds

| Level | Distance | Vertex Count | Use Case |
|-------|----------|--------------|----------|
| HIGH | < 50m | Full geometry | Close-up detail |
| MEDIUM | 50-150m | Reduced | Mid-range |
| LOW | > 150m | Minimal | Far distance |

### Object Count Presets

| Preset | Count | Target FPS | Notes |
|--------|-------|------------|-------|
| Minimal | 3,000 | 60+ | Development/debugging |
| Medium | 15,000 | 60 | Balanced quality |
| Maximum | 500,000 | 30-60 | Stress test (GPU-bound) |

### Culling

- **Distance Culling**: Default 500m (configurable)
- **Frustum Culling**: Via CullingSystem each frame
- **LOD Updates**: LODSystem updates levels based on camera distance

---

## Common Tasks

**Add a GLTF model:**
1. Place in `assets/models/`
2. Load: `modelManager.loadModel("assets/models/name.glb", "name")`
3. Create instance: `modelManager.addModelInstance("name", transform)`
4. PBR shader applies automatically

**Add shader uniforms:** Set via `shader.setUniform("name", value)`. Flashlight uses UBO at binding point 2.

**Tune performance:** Adjust object count presets in Constants.h, toggle culling/LOD in Performance UI tab.

**Subscribe to config changes:**
```cpp
m_fogSub = EventBus::instance().subscribe<FogConfigChangedEvent>(
    this, &MyClass::onFogChanged);
```

**Create an ECS entity:**
```cpp
auto entity = ECSWorld::getRegistry().create();
ECSWorld::getRegistry().add<TransformComponent>(entity, pos, rot, scale);
```

**Add a new event type:**
1. Define struct in `src/events/` inheriting from `EventBase<YourEvent>`
2. Subscribe in interested classes
3. Publish when state changes

---

## Debugging

### Debug Visualization
- **Grid**: Blender-style infinite grid at Y=0 (toggle in Viewport panel)
- **Axes**: RGB origin axes (X=red, Y=green, Z=blue)
- **Gizmo**: Corner orientation indicator

### Performance Monitoring
- FPS counter in status bar
- Object/triangle counts in Performance panel
- LOD distribution visualization

### Common Issues
- **Black screen**: Check shader compilation errors in console
- **Missing textures**: Ensure running from repository root
- **Low FPS**: Reduce object count, enable culling/LOD

---

## Project Status & Next Steps

*See [docs/STATUS.md](docs/STATUS.md) for detailed implementation status.*

### Current State (December 2024)

The engine is **~95% complete** and production-ready:
- Rendering pipeline works (500k objects, PBR, particles, fog)
- ECS architecture fully implemented with 6 systems running
- Event system complete and actively used
- Uniform caching implemented for performance
- ConfigManager has JSON persistence

### Recently Completed

1. **Uniform Location Caching** - All managers cache `glGetUniformLocation()` results
   - ObjectManager: 18 cached uniforms
   - ModelManager: 18 cached uniforms
   - LightManager: Per-shader cache for light arrays
   - Shader class: Hash map cache for all setUniform() calls

2. **ECS Systems Registered** - 6 systems now run in main loop:
   - TransformSystem, LODSystem, CullingSystem
   - AnimationSystem, ParticleSystem, LightSystem

3. **ConfigManager JSON Persistence** - Full save/load implemented
   - Saves all config sections to human-readable JSON
   - Publishes events on load for automatic system updates

### Remaining Work

**Medium Priority:**
1. **Remove UIManager State Duplication**
   - UIManager should read from ConfigManager, not maintain copies
   - Eliminates sync bugs between UI and actual state

**Low Priority (Future Enhancements):**
2. **Migrate to ECS-Only Storage** - Remove parallel data structures from managers
3. **Instance Buffer Optimization** - Persistent mapped buffers or compute shaders
4. **Integrate RenderSystem** - Use ECS RenderSystem instead of manager render calls
5. **Physics Integration** - Full Bullet3 rigid body sync

### TODOs in Code

```
ModelManager.cpp:130   - Remove instances when unloading model
PhysicsSystem.h:102    - Implement Bullet rigid body sync
```

### Documentation

| Document | Purpose |
|----------|---------|
| [CLAUDE.md](CLAUDE.md) | This file - development guide |
| [docs/STATUS.md](docs/STATUS.md) | Implementation status & component matrix |
| [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md) | System diagrams & data flow |
| [docs/REFACTORING_PROPOSAL.md](docs/REFACTORING_PROPOSAL.md) | Optimization plans with code examples |
