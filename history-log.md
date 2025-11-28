# OpenGL Avanzado - Development History Log

## Project Evolution Summary

- **Started**: OpenGL project with basic scene, camera, and lighting
- **Current State**: Advanced scene with FING model, floor with snow textures,
  flashlight system, and ImGui controls
- **Architecture**: Clean separation with Application, Scene, Camera,
  LightManager, ModelManager, ObjectManager, Renderer

## Major Milestones

### Phase 1: Foundation (Initial Setup)

- Basic OpenGL context with SDL2
- Simple scene with floor plane
- Camera controls (WASD + mouse)
- Basic Phong lighting

### Phase 2: Texture System & Snow Floor

- **Texture Change**: Switched from `snow-texture.png` to `Baked_snowflake.png`
- **Snow Texture Set**: Integrated `assets/snow/` textures (albedo, roughness,
  translucency, height)
- **Normal Mapping**: Implemented normal mapping on floor using height map
- **Material Controls**: Added ImGui menu (ESC) for runtime material tuning
  - Normal strength: 0.276 (default)
  - Specular strength: 0.5 (default)
  - Ambient, roughness bias controls

### Phase 3: Tessellation Experiment (Reverted)

- **Attempted**: Real tessellation with displacement mapping
- **Issues**: Floor disappearing, complex debugging
- **Decision**: Reverted to normal mapping approach
- **Files**: Created `phong_notess.vert/frag` for non-tessellated pipeline

### Phase 4: FING Model Integration

- **GLTF Support**: Added GLTFModel class with PBR shading
- **Model Positioning**: FING model positioned and exposed in ImGui
- **Transform Controls**: Runtime position/scale adjustment via UI

### Phase 5: Flashlight System

- **Implementation**: Added flashlight to LightManager
- **Integration**: PBR shader support for GLTF models
- **UI Controls**: Brightness, cutoff angle, color controls
- **Bug Fixes**:
  - Fixed `flashlightIndex` initialization (-1 instead of 0)
  - Fixed sign error in cone test
    (`dot(lightToFrag, normalize(uFlashlightDir))`)
  - Extended flashlight to floor and spawned objects
- **Current State**: Working across all scene elements

### Phase 6: UI & Event System

- **ImGui Integration**: ESC toggles menu, mouse focus management
- **Event Handling**: Proper separation of UI vs scene input
- **Performance**: Maintained 11-12 FPS with 500k objects

## Technical Architecture

### Core Classes

- **Application**: Main loop, event handling, ImGui integration
- **Scene**: Floor rendering, texture management, shader uniforms
- **Camera**: FPS-style movement, mouse look
- **LightManager**: Multiple lights, flashlight system
- **ModelManager**: GLTF model loading, PBR rendering
- **ObjectManager**: Instanced object spawning, LOD system
- **Renderer**: OpenGL context, window management

### Shader Pipeline

- **Floor**: `phong_notess.vert/frag` with normal mapping
- **GLTF Models**: `pbr_model.vert/frag` with PBR lighting
- **Objects**: Internal shader in ObjectManager
- **Lighting**: Unified flashlight system across all shaders

### Key Features

- **Normal Mapping**: Height-based normal perturbation
- **PBR Lighting**: Physically-based rendering for models
- **Flashlight**: Spotlight with configurable cone, brightness, color
- **ImGui Controls**: Runtime parameter tuning
- **Performance**: 500k objects with LOD and culling

## Current State (Pre-Snow)

- **Scene**: FING model + textured floor + 500k spawned objects
- **Lighting**: Multiple lights + configurable flashlight
- **UI**: ESC menu with material, model, and flashlight controls
- **Performance**: Stable 11-12 FPS
- **Textures**: Snow texture set with normal mapping

## Phase 7: Snow System Implementation

- **Target**: Minimal viable falling snow (5k flakes)
- **Implementation**:
  - Created `SnowSystem` class with GPU-instanced billboards
  - Adapted `snow_glow.vert/frag` shaders from prev_code
  - Integrated into Scene class with update/render pipeline
  - Added ImGui controls for all snow parameters
- **Features**:
  - 5k snowflakes by default (configurable 0-50k)
  - Wind simulation with direction and speed
  - Per-flake fall speed variation
  - Billboard rendering with camera-facing quads
  - Real-time parameter tuning via ImGui
- **Performance**: Maintains current FPS with snow addition
- **Files Added**: `SnowSystem.h/cpp`, `shaders/snow_glow.vert/frag`

## Phase 8: Performance Optimization & Culling

- **Culling System**: Made culling enabled by default
- **Snow Frustum Culling**: Added frustum culling to snow system
- **Implementation**:
  - Updated `SnowSystem` with frustum plane extraction and testing
  - Only renders visible snowflakes when culling enabled
  - Added culling settings to ImGui menu
- **Performance**: Significant improvement for high snowflake counts
- **UI**: Added "System Culling" section with object culling/LOD toggles

## Phase 9: Fog Shader System

- **Target**: Distance-based atmospheric fog affecting all scene elements
- **Algorithm**: Exponential fog using
  `fogFactor = 1 - exp(-density * distance)`
- **Implementation**:
  - Added fog uniforms to all fragment shaders:
    - `phong_notess.frag` (floor)
    - `pbr_model.frag` (GLTF models)
    - ObjectManager internal shader (spawned objects)
    - `snow_glow.frag` (snowflakes)
  - Integrated fog parameters into all rendering systems
  - Added comprehensive ImGui controls
- **Features**:
  - **Fog Controls**: Enable/disable, color picker, density slider
  - **Exponential Falloff**: Non-linear distance-based fog intensity
  - **Color Blending**: `mix(objectColor, fogColor, fogFactor)`
  - **Consistent Application**: All objects use same fog parameters
- **Default Settings**:
  - Fog: Disabled by default
  - Color: Black (0, 0, 0)
  - Density: 0.01 (subtle effect)
- **UI**: Added "Fog Settings" section with real-time controls

## Files Modified (Recent)

- `Application.cpp/h`: ImGui integration, flashlight UI, snow controls, fog
  controls
- `Scene.cpp/h`: Floor textures, normal mapping, flashlight uniforms, snow
  integration, fog parameters
- `LightManager.cpp/h`: Flashlight system, brightness/color controls
- `ModelManager.cpp/h`: PBR flashlight integration, fog support
- `ObjectManager.cpp/h`: Flashlight for spawned objects, fog support
- `SnowSystem.h/cpp`: Snow particle system with GPU instancing, frustum culling,
  fog support
- `shaders/phong_notess.frag`: Normal mapping, flashlight support, fog
  implementation
- `shaders/pbr_model.frag`: PBR flashlight implementation, fog support
- `shaders/snow_glow.vert/frag`: Snow billboard rendering shaders, fog support

## Changelog (Recent Deltas)

- [Batch 1] Snow transparency: Enabled blending and disabled depth writes for
  snow rendering; fixed GLM frustum plane extraction; gated debug logs behind
  `OPENGL_ADV_DEBUG_LOGS`.
- [Batch 2] Color pipeline: Enabled framebuffer sRGB; removed manual gamma in
  floor/PBR; corrected texture color spaces (albedo sRGB,
  MR/AO/height/translucency linear); fog applied in linear space.
- Defaults: Fog enabled by default across systems
  (Application/Scene/ModelManager/ObjectManager/SnowSystem).
- [Batch 3] ObjectManager: Switched to per-LOD instanced rendering; removed
  fragment distance discard; added instance buffers and instanced vertex shader.
  -. [Batch 4] Flashlight: Improved PBR flashlight with smooth inner/outer cone;
  added unified Flashlight UBO and shader-side support (UBO coexists with legacy
  uniforms); ensured GL resource creation after context initialization.

- [Bullet3 MVP] Minimal ground collision for snowflakes:
  - Added UI toggle "Bullet Ground Collision (snow)" in Application menu
    (default ON).
  - Scene forwards toggle to SnowSystem.
  - SnowSystem embeds a tiny Bullet world with a static ground plane at Y=0.
  - Per-frame ray tests clamp flakes to ground and immediately respawn to keep
    density.
  - Default off; no behavioral change unless enabled.

## Performance Notes

- **Object Count**: 500k spawned objects
- **Snow Count**: 5k-30k snowflakes (configurable)
- **FPS**: 11-12 FPS (target maintained)
- **Memory**: Efficient instancing and LOD
- **Culling**: Frustum and distance culling active
- **Fog**: Per-fragment calculation, minimal performance impact

## Guidance & Notes

- Legacy Directory: The `prev_code/` directory is a previous iteration and is
  ignored for the current codebase moving forward.

### Phase 10: Rendering Stability Batch (Planned)

- Snow blending and depth policy: Enable alpha blending; keep depth test on;
  disable depth writes while rendering snow, then restore. Ensures correct snow
  transparency.
- Snow frustum culling fix: Correct frustum plane extraction for GLM
  column-major matrices to improve visible counts and performance.
- Debug logging gating: Add runtime log level and avoid periodic prints in
  render paths for non-debug builds.

### Phase 11: Unified Color Pipeline (Completed)

- Enabled `GL_FRAMEBUFFER_SRGB` and kept lighting in linear space.
- Removed manual gamma pow conversions; baseColor sampled from sRGB textures
  (auto-linearized).
- Ensured MR/AO/height/translucency use linear formats; GLTF baseColor uses
  sRGB; MR/AO linear.
- Applied fog in linear space across floor, PBR, and snow shaders.
- Result: consistent visuals and predictable brightness across pipelines.

### Phase 12: Object Instancing & Culling (Planned)

- Switch `ObjectManager` to instanced draws per LOD (mat4 instance buffer) to
  reduce CPU draw overhead drastically.
- Remove fragment-side distance discards once CPU/LOD/frustum culling is
  authoritative.
- Expose and align LOD/cull distances via UI and `Scene` so thresholds are
  consistent.

### Phase 13: Flashlight Uniforms Unification (Planned)

- Introduce a small `Flashlight` UBO (position, direction, cutoff cosine,
  brightness, color, enabled) and bind across all pipelines (floor, PBR models,
  objects).
- Standardize cone, falloff, and attenuation so flashlight behaves identically
  everywhere.

### Phase 14: PBR Enhancements (Completed)

- **PBR Normal Map Groundwork**: Added tangent attribute parsing in GLTFModel,
  normal texture loading, and basic normal map perturbation in PBR shader
- **Enhanced Flashlight**: Improved PBR flashlight with smoother cone falloff
  and broader specular highlights
- **UBO Migration**: Completed flashlight UBO migration across all shaders for
  consistent behavior

### Phase 15: Enhanced Fog System (Completed)

- **Fog Desaturation**: Added desaturation parameter to all fog implementations
  across shaders
- **Realistic Atmospheric Perspective**: Implemented quadratic desaturation
  curve where distant objects lose color faster than they get tinted
- **Separate Controls**: Added UI control for fog desaturation strength (0.0-2.0
  range)
- **Shader Updates**: Enhanced fog in phong_notess.frag, pbr_model.frag, and
  snow_glow.frag with two-stage process:
  1. Desaturate based on distance (stronger effect)
  2. Apply fog tinting (weaker effect, 60% strength)
- **C++ Integration**: Added fogDesaturationStrength parameter to Scene,
  ObjectManager, ModelManager, and SnowSystem classes
