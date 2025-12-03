# Architecture Overview

This document describes the high-level architecture of the OpenGL graphics engine.

## System Diagram

```
                                    +------------------+
                                    |   Application    |
                                    |  (Main Loop)     |
                                    +--------+---------+
                                             |
              +------------------------------+------------------------------+
              |                              |                              |
              v                              v                              v
      +-------+-------+             +--------+--------+            +--------+--------+
      |   Renderer    |             |     Camera      |            |  LightManager   |
      | (SDL2/OpenGL) |             | (FPS Controls)  |            | (UBO @ bind 2)  |
      +-------+-------+             +--------+--------+            +--------+--------+
              |                              |                              |
              |                              v                              |
              |                     +--------+--------+                     |
              +-------------------->|     IScene      |<--------------------+
                                    | (Scene Interface)|
                                    +--------+--------+
                                             |
                          +------------------+------------------+
                          |                                     |
                          v                                     v
                  +-------+-------+                     +-------+-------+
                  |   BaseScene   |                     |   DemoScene   |
                  | (Floor, Fog,  |                     | (Full Demo)   |
                  |  DebugRender) |                     +-------+-------+
                  +---------------+                             |
                                              +-----------------+-----------------+
                                              |                 |                 |
                                              v                 v                 v
                                      +-------+-----+   +-------+-----+   +-------+-----+
                                      |ObjectManager|   |ModelManager |   | SnowSystem  |
                                      | (500k inst) |   | (GLTF/PBR)  |   | (Particles) |
                                      +-------------+   +-------------+   +-------------+

                          +-----------------------------------------------------------+
                          |                        ECSWorld                           |
                          |  Registry (entities + components) | SystemScheduler       |
                          +-----------------------------------------------------------+
                                    |                                     |
              +---------------------+---------------------+               |
              |                     |                     |               |
              v                     v                     v               v
      +-------+-------+     +-------+-------+     +-------+-------+  +----+----+
      |  Transform    |     |  Renderable   |     |     LOD       |  | Systems |
      |  Component    |     |  Component    |     |  Component    |  | (LOD,   |
      +---------------+     +---------------+     +---------------+  | Culling)|
                                                                     +---------+

                          +-----------------------------------------------------------+
                          |                       EventBus                            |
                          |         subscribe<T>() / publish<T>() / queue<T>()        |
                          +-----------------------------------------------------------+
                                    ^                                     ^
                                    |                                     |
                          +---------+---------+                 +---------+---------+
                          |   ConfigManager   |                 |   InputManager    |
                          | (Settings Source) |                 | (SDL Events)      |
                          +-------------------+                 +-------------------+
                                    ^
                                    |
                          +---------+---------+
                          |    UIManager      |
                          | (ImGui Panels)    |
                          +-------------------+
```

## Initialization Sequence

```
main()
  |
  +-> Application::initialize()
        |
        +-> Renderer::initialize(width, height)
        |     +-> SDL_Init(SDL_INIT_VIDEO)
        |     +-> SDL_CreateWindow() [maximized, resizable]
        |     +-> SDL_GL_CreateContext() [OpenGL 4.5 core]
        |     +-> gladLoadGLLoader()
        |     +-> setupOpenGL() [depth, cull, sRGB]
        |     +-> Load overlay shaders (snow trail effect)
        |     +-> Create accumulation FBOs
        |
        +-> InputManager::initialize(window)
        |     +-> Store SDL_Window reference
        |     +-> Set event preprocessor for ImGui
        |
        +-> ECSWorld::initialize()
        |     +-> Create global Registry
        |     +-> Create SystemScheduler
        |     +-> Register systems (LODSystem, CullingSystem, etc.)
        |
        +-> Scene::initialize()
        |     +-> BaseScene::initialize()
        |     |     +-> setupFloorGeometry()
        |     |     +-> setupFloorShader()
        |     |     +-> Load floor textures
        |     |     +-> DebugRenderer::initialize()
        |     |     +-> Subscribe to config events
        |     |
        |     +-> [DemoScene only]
        |           +-> ObjectManager::initialize(objectCount)
        |           +-> ModelManager::initialize()
        |           +-> SnowSystem::initialize()
        |
        +-> ImGui initialization
        |     +-> IMGUI_CHECKVERSION()
        |     +-> ImGui::CreateContext()
        |     +-> ImGui_ImplSDL2_InitForOpenGL()
        |     +-> ImGui_ImplOpenGL3_Init()
        |
        +-> UIManager::initialize()
        |
        +-> LightManager::initializeGLResources()
              +-> Create flashlight UBO (binding point 2)
```

## Main Loop

```
Application::run()
  |
  +-> while (running)
        |
        +-> PerformanceProfiler::startFrame()
        |
        +-> handleEvents()
        |     +-> InputManager::processEvents()
        |           +-> SDL_PollEvent() loop
        |           +-> Publish: KeyPressed, MouseMoved, WindowResized, etc.
        |           +-> Check ImGui capture flags
        |
        +-> Calculate deltaTime (SDL performance counter)
        |
        +-> update(deltaTime)
        |     +-> Camera input handling (WASD + mouse)
        |     |     +-> camera.handleInput(keys, deltaTime)
        |     |     +-> camera.handleMouseInput(deltaX, deltaY)
        |     |
        |     +-> ECSWorld::update(deltaTime)
        |     |     +-> LODSystem::update() [update LOD levels]
        |     |     +-> CullingSystem::update() [update visibility]
        |     |
        |     +-> Scene::update(cameraPos, deltaTime, view, projection)
        |     |     +-> [DemoScene: ObjectManager, ModelManager, SnowSystem update]
        |     |
        |     +-> Flashlight sync
        |           +-> lightManager.updateFlashlight(cameraPos, cameraFront)
        |           +-> lightManager.updateFlashlightUBO()
        |
        +-> render()
        |     +-> ImGui::NewFrame()
        |     +-> ImGui_ImplOpenGL3_NewFrame()
        |     +-> ImGui_ImplSDL2_NewFrame()
        |     |
        |     +-> UIManager::renderMenuBar()
        |     +-> UIManager::renderSidebar()
        |     +-> UIManager::renderStatusBar()
        |     |
        |     +-> Apply UI state to scene/managers
        |     |     +-> scene.setFog*(), setMaterial*(), setDebug*()
        |     |     +-> lightManager.setFlashlight*()
        |     |
        |     +-> Renderer::render(camera, scene, lightManager)
        |     |     +-> glClear(COLOR | DEPTH)
        |     |     +-> scene.render(view, projection, cameraPos, cameraFront, lightManager)
        |     |           +-> BaseScene::renderFloor()
        |     |           +-> DebugRenderer::render() [grid, axes, gizmo]
        |     |           +-> [DemoScene: ObjectManager, ModelManager, SnowSystem render]
        |     |     +-> [Optional: overlay snow trail effect]
        |     |
        |     +-> ImGui::Render()
        |     +-> ImGui_ImplOpenGL3_RenderDrawData()
        |     +-> SDL_GL_SwapWindow()
        |
        +-> PerformanceProfiler::endFrame()
```

## Render Pipeline

```
                        +------------------+
                        | Clear Framebuffer|
                        | (Color + Depth)  |
                        +--------+---------+
                                 |
                                 v
+----------------+      +--------+---------+      +----------------+
|                |      |   Scene Render   |      |                |
| Floor Shader   |<-----|                  |----->| Object Shader  |
| (phong_notess) |      |                  |      |(instanced x3)  |
+----------------+      +--------+---------+      +----------------+
                                 |
              +------------------+------------------+
              |                                     |
              v                                     v
      +-------+-------+                     +-------+-------+
      | Model Shader  |                     | Snow Shader   |
      | (pbr_model)   |                     | (snow_glow)   |
      +---------------+                     +---------------+
              |                                     |
              +------------------+------------------+
                                 |
                                 v
                        +--------+---------+
                        |  Debug Shaders   |
                        | (grid, axes,     |
                        |  gizmo)          |
                        +--------+---------+
                                 |
                                 v
                        +--------+---------+
                        | [Optional]       |
                        | Snow Overlay     |
                        | + Accumulation   |
                        +--------+---------+
                                 |
                                 v
                        +--------+---------+
                        |   ImGui Render   |
                        +--------+---------+
                                 |
                                 v
                        +--------+---------+
                        | SDL_GL_SwapWindow|
                        +------------------+
```

## Event Propagation

```
User Input (SDL)
      |
      v
+-----+------+     +-----------------+     +------------------+
|InputManager|---->|    EventBus     |---->|   Subscribers    |
+------------+     +-----------------+     +------------------+
                          |                        |
                          |   KeyPressedEvent      +-> Application::onKeyPressed
                          |   MouseMovedEvent      +-> Application::onCameraLook
                          |   WindowResizedEvent   +-> Application::onWindowResized
                          |
                          v
                   +-----------------+
                   |  UIManager      |
                   | (Tab, Esc, L)   |
                   +--------+--------+
                            |
                            | User changes setting via ImGui
                            v
                   +--------+--------+
                   | ConfigManager   |
                   | (setFog*,       |
                   |  setSnow*, etc) |
                   +--------+--------+
                            |
                            | Publishes config events
                            v
                   +--------+--------+
                   |    EventBus     |
                   +--------+--------+
                            |
        +-------------------+-------------------+
        |                   |                   |
        v                   v                   v
+-------+-------+   +-------+-------+   +-------+-------+
| ObjectManager |   | ModelManager  |   |  SnowSystem   |
| onFogChanged  |   |               |   | onSnowChanged |
+---------------+   +---------------+   +---------------+
        |                   |                   |
        v                   v                   v
   Update fog           Update fog          Update snow
   uniforms             uniforms            parameters
```

## Manager Dependencies

```
                          Application
                              |
        +---------------------+---------------------+
        |                     |                     |
        v                     v                     v
    Renderer              Camera              LightManager
        |                     |                     |
        +----------+----------+                     |
                   |                                |
                   v                                |
                IScene <----------------------------+
                   |
        +----------+----------+
        |                     |
        v                     v
    BaseScene            DemoScene
        |                     |
        v                     +----------+----------+----------+
  DebugRenderer               |          |          |          |
                              v          v          v          v
                        ObjectMgr   ModelMgr   SnowSystem   (Floor from BaseScene)
                              |          |          |
                              v          v          v
                           ECSWorld (shared registry)
                              |
                    +---------+---------+
                    |                   |
                    v                   v
               LODSystem          CullingSystem
```

## Shader Bindings

### Uniform Buffer Objects (UBOs)

| Binding Point | Name | Contents | Used By |
|---------------|------|----------|---------|
| 2 | FlashlightBlock | position, direction, color, brightness, cutoff | pbr_model, object_instanced, phong_notess |

### Common Uniforms

All primary fragment shaders share:

```glsl
// Fog (set by each manager's render method)
uniform bool  uFogEnabled;
uniform vec3  uFogColor;
uniform float uFogDensity;
uniform float uFogDesaturationStrength;
uniform float uFogAbsorptionDensity;
uniform float uFogAbsorptionStrength;
uniform vec3  uBackgroundColor;

// Flashlight (alternative to UBO for compatibility)
uniform vec3  uFlashlightPos;
uniform vec3  uFlashlightDir;
uniform float uFlashlightCutoff;
uniform float uFlashlightBrightness;
uniform vec3  uFlashlightColor;
uniform bool  uFlashlightOn;
```

### Vertex Attributes

| Shader | Location 0 | Location 1 | Location 2 | Location 3+ |
|--------|------------|------------|------------|-------------|
| pbr_model | Position | Normal | Color | TexCoord, Tangent, Weights, Joints |
| object_instanced | Position | Normal | UV | Instance Mat4 (3,4,5,6) |
| snow_glow | Position | Center | Seed | - |
| phong_notess | Position | Normal | UV | - |
| debug_grid | Position | - | - | - |

## Performance Bottlenecks

### Known CPU Bottlenecks

1. **Uniform Location Lookup** (every frame)
   - `glGetUniformLocation()` called 50+ times per frame
   - Should cache locations after shader compilation
   - Files: ObjectManager.cpp, ModelManager.cpp, SnowSystem.cpp

2. **Instance Buffer Rebuild**
   - Full `glBufferSubData()` for all visible entities
   - Could use persistent mapped buffers
   - Files: ObjectManager.cpp, SnowSystem.cpp

3. **Entity Iteration**
   - Linear scan of all entities to gather visible ones
   - Could cache visible entity lists
   - Files: ObjectManager::gatherInstanceMatrices()

### GPU Considerations

1. **LOD System**
   - 3 levels reduce vertex count for distant objects
   - Thresholds: HIGH < 50m, MEDIUM < 150m, LOW > 150m

2. **Frustum Culling**
   - Distance-based culling via CullingSystem
   - Default cull distance: 500m

3. **Instanced Rendering**
   - ObjectManager: Single draw call per LOD level
   - SnowSystem: Single draw call for all particles

## Thread Safety

The engine is **single-threaded**. All operations occur on the main thread:

- SDL event processing
- ECS updates
- Rendering
- ImGui

No mutex or thread synchronization is used. If multithreading is added:
- ECSWorld would need concurrent access protection
- EventBus subscriptions would need locking
- GPU resource creation must remain on main thread
