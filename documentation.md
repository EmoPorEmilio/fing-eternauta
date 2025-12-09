# Fing-Eternauta: Computer Graphics Technical Documentation

## Project Overview

A real-time 3D game engine built from scratch using OpenGL 4.5, featuring a third-person character controller navigating a procedurally generated city. This document details all computer graphics concepts, algorithms, and structures implemented.

---

## 1. Rendering Pipeline

### 1.1 Graphics API
- **OpenGL 4.5 Core Profile**: Modern programmable pipeline with no fixed-function fallbacks
- **GLAD**: OpenGL loader for function pointer resolution
- **SDL3**: Window creation, OpenGL context management, and input handling

### 1.2 Vertex Processing

#### Vertex Buffer Objects (VBO)
All geometry is stored in GPU memory using VBOs with interleaved vertex attributes:
- **Position** (vec3): World-space vertex coordinates
- **Normal** (vec3): Surface normal for lighting calculations
- **Texture Coordinates** (vec2): UV mapping for texture sampling
- **Bone Indices** (ivec4): For skeletal animation (skinned meshes)
- **Bone Weights** (vec4): Blend weights for skeletal animation

#### Vertex Array Objects (VAO)
VAOs encapsulate vertex attribute state, enabling fast geometry switching during rendering.

#### Element Buffer Objects (EBO)
Indexed rendering using EBOs reduces vertex duplication and GPU memory usage.

### 1.3 Shader Programs

#### Vertex Shaders
Transform vertices from model space to clip space through the transformation pipeline:
```
Model Space → World Space → View Space → Clip Space → NDC → Screen Space
```

**Transformation Matrices:**
- **Model Matrix**: Object-to-world transformation (position, rotation, scale)
- **View Matrix**: World-to-camera transformation using `glm::lookAt()`
- **Projection Matrix**: Perspective projection with configurable FOV, near/far planes

#### Fragment Shaders
Compute per-pixel color output with:
- **Texture Sampling**: `texture()` function with sampler2D
- **Lighting Calculations**: Diffuse + ambient illumination
- **Fog Effects**: Exponential squared distance fog
- **Alpha Blending**: For UI elements and transparency

---

## 2. Lighting Model

### 2.1 Phong Illumination (Simplified)

**Ambient Lighting:**
```glsl
float ambient = 0.3;
```
Constant term providing base illumination to prevent completely black shadows.

**Diffuse Lighting (Lambertian Reflectance):**
```glsl
float diff = max(dot(normal, lightDir), 0.0);
```
Light intensity proportional to the angle between surface normal and light direction. Uses the dot product clamped to [0,1].

**Combined Lighting:**
```glsl
float light = ambient + diff * 0.7;
vec3 litColor = textureColor * light;
```

### 2.2 Directional Light
Single infinite-distance light source simulating sunlight:
```cpp
glm::vec3 lightDir = glm::normalize(glm::vec3(0.5f, 1.0f, 0.3f));
```

---

## 3. Texture Mapping

### 3.1 2D Texture Mapping
- **UV Coordinates**: Per-vertex texture coordinates mapping geometry to texture space
- **Texture Wrapping**: `GL_REPEAT` for tiled ground textures
- **Texture Filtering**:
  - `GL_LINEAR_MIPMAP_LINEAR` for minification (trilinear filtering)
  - `GL_LINEAR` for magnification (bilinear filtering)

### 3.2 Mipmapping
Automatic mipmap generation with `glGenerateMipmap()` for:
- Reduced aliasing at distance
- Improved texture cache performance
- Anisotropic filtering compatibility

### 3.3 Tiled/Repeating Textures
Ground plane uses scaled UV coordinates for seamless tiling:
```cpp
float uvScale = planeSize * texScale;  // World units to UV scale
```

---

## 4. Skeletal Animation

### 4.1 Bone Hierarchy
- **Joint Tree**: Hierarchical skeleton structure loaded from glTF/GLB files
- **Bind Pose**: Reference pose where skin is attached to skeleton
- **Inverse Bind Matrices**: Transform from mesh space to bone space

### 4.2 Keyframe Interpolation
Animation data stored as keyframes with:
- **Translation Keys**: Position over time
- **Rotation Keys**: Quaternion orientation over time
- **Scale Keys**: Uniform/non-uniform scaling over time

**Linear Interpolation (LERP)** for position/scale:
```cpp
glm::mix(key1.value, key2.value, t);
```

**Spherical Linear Interpolation (SLERP)** for rotations:
```cpp
glm::slerp(quat1, quat2, t);
```

### 4.3 Skinning (Linear Blend Skinning / LBS)
Each vertex influenced by up to 4 bones:
```glsl
mat4 skinMatrix = uBones[boneIndices.x] * boneWeights.x
                + uBones[boneIndices.y] * boneWeights.y
                + uBones[boneIndices.z] * boneWeights.z
                + uBones[boneIndices.w] * boneWeights.w;
vec4 skinnedPos = skinMatrix * vec4(aPosition, 1.0);
```

### 4.4 Bone Matrix Computation
```cpp
boneMatrix = globalTransform * inverseBindMatrix;
```
Transforms vertices from bind pose to animated pose.

---

## 5. Camera System

### 5.1 Third-Person Camera
Over-the-shoulder view with configurable parameters:
- **Distance**: Camera distance behind target
- **Height**: Vertical offset above target
- **Shoulder Offset**: Horizontal offset for asymmetric framing
- **Look Ahead**: Point of interest ahead of character

### 5.2 View Matrix Construction
```cpp
glm::mat4 view = glm::lookAt(cameraPosition, lookAtTarget, upVector);
```

### 5.3 Perspective Projection
```cpp
glm::mat4 projection = glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
```
- **Field of View**: 60 degrees
- **Near Plane**: 0.1 units (prevents z-fighting)
- **Far Plane**: 100 units (rendering distance limit)

### 5.4 Camera Orbit Controls
- **Yaw**: Horizontal rotation (mouse X movement)
- **Pitch**: Vertical rotation (mouse Y movement) clamped to [-60, +60] degrees

---

## 6. Procedural Geometry Generation

### 6.1 Box Mesh Generation
Procedural unit cube (1x1x1) with:
- **24 vertices**: 4 per face for correct flat shading normals
- **36 indices**: 12 triangles (2 per face)
- **Face Normals**: Each face has uniform normal direction

```cpp
// 6 faces × 4 vertices × 8 floats (pos + normal + uv)
std::vector<float> vertices = { ... };
```

### 6.2 Procedural City Grid
100×100 building grid (10,000 buildings) with:
- **Random Heights**: Uniform distribution [15, 40] units
- **Street Gaps**: 12-unit spacing between buildings
- **Transform Scaling**: Single unit-cube mesh scaled per-building

---

## 7. Spatial Data Structures & Culling

### 7.1 Grid-Based Spatial Culling
Efficient O(1) lookup for nearby entities using grid coordinates:
```cpp
int gridX = floor((position.x - offset) / blockSize);
int gridZ = floor((position.z - offset) / blockSize);
```

### 7.2 Distance-Based Culling
Only buildings within a configurable grid radius are rendered:
- **Render Radius**: 3 cells (7×7 = 49 buildings visible)
- **Chebyshev Distance**: `max(|dx|, |dz|) <= radius`

### 7.3 Entity Pool Pattern
Fixed pool of rendering entities reused for visible buildings:
- Avoids creating/destroying entities at runtime
- Buildings outside range are hidden (Y = -1000)
- Pool size matches maximum visible count

### 7.4 AABB (Axis-Aligned Bounding Box)
Bounding volume for spatial queries:
```cpp
struct AABB {
    glm::vec3 min;
    glm::vec3 max;
};
```

---

## 8. Atmospheric Effects

### 8.1 Exponential Squared Fog
Distance-based fog using exponential squared falloff:
```glsl
float distance = length(fragPos - viewPos);
float fogFactor = 1.0 - exp(-pow(fogDensity * distance, 2.0));
fogFactor = clamp(fogFactor, 0.0, 1.0);
```

### 8.2 Fog Blending
Color interpolation with desaturation effect:
```glsl
float gray = luminance(litColor);  // Grayscale conversion
vec3 desaturated = mix(litColor, vec3(gray), fogFactor * fogDesaturation);
vec3 finalColor = mix(desaturated, fogColor, fogFactor);
```

### 8.3 Fog Parameters
- **Fog Color**: Matches background (0.1, 0.1, 0.12)
- **Fog Density**: 0.02 (configurable via pause menu)
- **Desaturation Factor**: 0.8 (reduces saturation with distance)

---

## 9. Level of Detail (LOD)

### 9.1 Distance-Based LOD Switching
Mesh quality selection based on camera distance:
```cpp
float distance = glm::length(cameraPos - objectPos);
bool useHighDetail = distance < lodSwitchDistance;  // 70 units
```

### 9.2 LOD Mesh Swapping
Hot-swapping mesh references without entity recreation:
```cpp
meshGroup->meshes = useHighDetail ? highDetailMeshes : lowDetailMeshes;
```

---

## 10. User Interface Rendering

### 10.1 Orthographic Projection
2D UI overlay using screen-space coordinates:
```cpp
glm::mat4 uiProjection = glm::ortho(0.0f, screenWidth, 0.0f, screenHeight);
```

### 10.2 Text Rendering
- **SDL_ttf**: TrueType font rasterization
- **Texture Caching**: Pre-rendered text textures for performance
- **Anchor Points**: Flexible positioning (Center, BottomCenter, etc.)

### 10.3 Minimap Rendering

#### Circular Minimap
Fragment shader discards pixels outside circle:
```glsl
if (length(fragPos - center) > radius) discard;
```

#### Building Footprints
Rotated rectangles rendered with per-building transform:
```glsl
vec2 rotated;
rotated.x = scaled.x * cos(rotation) - scaled.y * sin(rotation);
rotated.y = scaled.x * sin(rotation) + scaled.y * cos(rotation);
```

#### Cardinal Direction Indicators
Text markers (N, E, S, W) rotating with player orientation.

### 10.4 Alpha Blending
UI transparency using standard blend function:
```cpp
glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
```

---

## 11. Collision Detection

### 11.1 AABB vs Point
Simple containment test for ground collision:
```cpp
bool grounded = (position.y <= groundLevel);
```

### 11.2 Box Colliders
Axis-aligned box collision volumes with half-extents and offset.

---

## 12. Physics Simulation

### 12.1 Euler Integration
Simple velocity integration:
```cpp
velocity.y += gravity * dt;
position += velocity * dt;
```

### 12.2 Ground Clamping
Position correction on ground contact:
```cpp
if (position.y < groundLevel) {
    position.y = groundLevel;
    velocity.y = 0;
    grounded = true;
}
```

---

## 13. Entity Component System (ECS)

### 13.1 Architecture
- **Entities**: Unique integer identifiers
- **Components**: Plain data structures (Transform, Mesh, Renderable, etc.)
- **Systems**: Stateless processors operating on component groups

### 13.2 Component Types
| Component | Data |
|-----------|------|
| Transform | position, rotation (quaternion), scale |
| MeshGroup | VAO, index count, texture handles |
| Renderable | shader type, mesh offset |
| Camera | FOV, near/far planes, active flag |
| Skeleton | bone matrices, joint hierarchy |
| Animation | clip index, time, playing state |
| RigidBody | velocity, gravity, grounded flag |
| PlayerController | move speed, turn speed |
| FacingDirection | yaw angle, turn speed |
| FollowTarget | camera-player relationship |

### 13.3 Render System
Iterates all entities with Transform + MeshGroup + Renderable, setting uniforms and issuing draw calls.

---

## 14. Model Loading (glTF/GLB)

### 14.1 glTF Format
Industry-standard 3D format supporting:
- Mesh geometry with indexed primitives
- PBR materials and textures
- Skeletal animation with bone hierarchy
- Binary blob storage (GLB)

### 14.2 Data Extraction
- **Accessors**: Typed views into binary buffers
- **Buffer Views**: Byte ranges within buffers
- **Attribute Mapping**: Position, normal, UV, bone data

---

## 15. Depth Testing

### 15.1 Z-Buffer Algorithm
Hardware depth testing for correct occlusion:
```cpp
glEnable(GL_DEPTH_TEST);
```

### 15.2 Depth Buffer Clearing
Per-frame depth buffer reset:
```cpp
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
```

---

## 16. Shader Uniform Types

| Uniform | Type | Purpose |
|---------|------|---------|
| uModel | mat4 | Model transformation matrix |
| uView | mat4 | View transformation matrix |
| uProjection | mat4 | Projection matrix |
| uBones[] | mat4[] | Bone transformation array |
| uLightDir | vec3 | Directional light direction |
| uViewPos | vec3 | Camera position (for fog) |
| uTexture | sampler2D | Diffuse texture |
| uHasTexture | int | Texture presence flag |
| uFogEnabled | int | Fog toggle |
| uUseSkinning | int | Skeletal animation toggle |

---

## 17. Coordinate Systems

### 17.1 Spaces Used
- **Model Space**: Object-local coordinates
- **World Space**: Global scene coordinates (Y-up, right-handed)
- **View Space**: Camera-relative coordinates
- **Clip Space**: Homogeneous coordinates after projection
- **NDC**: Normalized Device Coordinates [-1, 1]
- **Screen Space**: Pixel coordinates

### 17.2 Quaternion Rotations
All rotations stored as quaternions to avoid gimbal lock:
```cpp
glm::quat rotation = glm::angleAxis(radians, axis);
transform.rotation = glm::slerp(current, target, t);
```

---

## 18. Double Buffering

Swap chain with back buffer rendering:
```cpp
SDL_GL_SwapWindow(window);
```
Prevents screen tearing and provides smooth frame presentation.

---

## 19. File Structure

```
src/
├── ecs/
│   ├── components/     # Data-only structs
│   ├── systems/        # Processing logic
│   └── Registry.h      # Entity-component storage
├── assets/
│   └── AssetLoader.cpp # glTF/GLB parsing
├── procedural/
│   └── BuildingGenerator.h  # Procedural mesh generation
├── spatial/
│   └── SpatialGrid.h   # Spatial culling structures
├── ui/
│   ├── FontManager.h   # Font loading
│   └── TextCache.h     # Text texture caching
└── Shader.h            # Shader compilation/linking

shaders/
├── model.vert/frag     # Standard lit shader
├── skinned.vert        # Skeletal animation shader
├── color.vert/frag     # Debug/solid color shader
├── ui.vert/frag        # 2D UI rendering
├── minimap.vert/frag   # Circular minimap
├── minimap_rect.vert/frag  # Building footprints
└── terrain.vert/frag   # Ground plane shader
```

---

## 20. Performance Optimizations

1. **Shared Mesh References**: 10,000 buildings share one VAO
2. **Entity Pooling**: Fixed allocation, no runtime allocs
3. **Grid-Based Culling**: O(1) visibility determination
4. **Dirty Flag Updates**: Culling only recomputes on cell change
5. **Texture Caching**: Pre-rendered UI text
6. **Mipmapping**: Reduced texture bandwidth at distance
7. **Early Fragment Discard**: Minimap circle clipping in shader

---

## 21. Libraries Used

| Library | Purpose |
|---------|---------|
| OpenGL 4.5 | Graphics API |
| GLAD | OpenGL loader |
| SDL3 | Windowing, input, context |
| SDL3_ttf | Font rendering |
| GLM | Math library (vectors, matrices, quaternions) |
| stb_image | Image loading |
| cgltf | glTF/GLB parsing |
